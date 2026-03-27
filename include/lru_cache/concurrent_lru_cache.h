#pragma once

#include "lru_cache/lru_cache.h"

#include <cstddef>
#include <optional>
#include <mutex>
#include <functional>
#include <memory>
#include <vector>

namespace bmngxn {

template<typename Key, typename Value, std::size_t NumShards = 128>
class concurrent_lru_cache {   
private:
    // std::hardware_destructive_interference_size might still cause false-sharing since 
    // each shard contains a mutex and an lru_cache, which together are likely larger than 64 bytes, 
    // -> we might want to over-align (128/256 bytes) to be safe and avoid false sharing between shards 
    // that are accessed very frequently
    struct alignas(256) Shard {
        std::mutex mutex;
        lru_cache<Key, Value> cache;

        explicit Shard(std::size_t capacity)
            : cache(capacity)
        {}
    };

    // TODO: alternatives for unique_ptr, which currently causes ptr chaisng
    std::vector<std::unique_ptr<Shard>> shards_;
    std::hash<Key> hasher_;

    Shard& get_shard(const Key& key) {
        std::size_t hash_val = hasher_(key) & (NumShards - 1);
        // ~ std::size_t hash_val = hasher_(key) % NumShards; (use AND instead of modulus if NumShards is a power of 2 -> fewer cycles)
        return *shards_[hash_val];
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
        shard.cache.put(key, value);
    }

    std::optional<Value> get(const Key& key) {
        Shard& shard = get_shard(key);
        std::lock_guard<std::mutex> lock(shard.mutex);
        
        Value* ptr = shard.cache.get(key);
        if (ptr != nullptr) {
            return *ptr;
        }
        return std::nullopt;
    }

    bool erase(const Key& key) {
        Shard& shard = get_shard(key);
        std::lock_guard<std::mutex> lock(shard.mutex);
        return shard.cache.erase(key);
    }
};

}
