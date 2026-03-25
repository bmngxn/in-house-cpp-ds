#pragma once

#include "lru_cache/lru_cache.h"

#include <cstddef>
#include <optional>
#include <mutex>
#include <functional>
#include <memory>
#include <vector>

namespace bmngxn {

template<typename Key, typename Value, std::size_t NumShards = 16>
class concurrent_lru_cache {   
private:
    struct alignas(std::hardware_destructive_interference_size) Shard {
        std::mutex mutex;
        lru_cache<Key, Value> lru_cache;

        explicit Shard(std::size_t capacity)
            : lru_cache(capacity)
        {}
    };

    // TODO: alternatives for unique_ptr, which currently causes ptr chaisng
    std::vector<std::unique_ptr<Shard>> shards_;
    std::hash<Key> hasher_;

    Shard& get_shard(const Key& key) {
        std::size_t hash_val = hasher_(key);
        return *shards_[hash_val % NumShards];
    }

public:
    explicit concurrent_lru_cache(std::size_t total_capacity) {
        std::size_t cap_per_shard = (total_capacity + NumShards - 1) / NumShards;
        shards_.reserve(NumShards);
        for (std::size_t i = 0; i < NumShards; i++) {
            shards_.push_back(std::make_unique<Shard>(cap_per_shard));
        }
    }

    void put(const Key& key, const Value& value) {
        Shard& shard = get_shard(key);
        std::lock_guard<std::mutex> lock(shard.mutex);
        shard.lru_cache.put(key, value);
    }

    std::optional<Value> get(const Key& key) {
        Shard& shard = get_shard(key);
        std::lock_guard<std::mutex> lock(shard.mutex);
        return shard.lru_cache.get(key);
    }

    bool erase(const Key& key) {
        Shard& shard = get_shard(key);
        std::lock_guard<std::mutex> lock(shard.mutex);
        return shard.lru_cache.erase(key);
    }
};

}
