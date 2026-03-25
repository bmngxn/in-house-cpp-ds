#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>

namespace bmngxn {
/**
 * Considerations to make this thread safe:
 * 
 * - get() actually is not only a read. its also a write (::splice()),
 * so shared_mutex might not be plausible
 * 
 * - gemini suggestion: lock sharding? 
 * 
 * - future: pre-allocated std::vector replace the std::list
 * 
 */
template <typename Key, typename Value>
class lru_cache {
private:
    // we'll use a std::list to keep track of the LRU property:
    // MRU at front, LRU at back. This also supports O(1)
    // insert/delete/reorder(splice), even though the cache locality is poor
    //
    // unordered_map for O(1) key lookup
    struct Node {
        Key key;
        Value value;

        Node(Key k, Value v)
            : key(k)
            , value(v)
        {}
    };

    std::size_t capacity_;
    std::list<Node> lines_;

    using LineIt = typename std::list<Node>::iterator;
    std::unordered_map<Key, LineIt> cache_;

public:
    explicit lru_cache(std::size_t capacity) noexcept
        : capacity_(capacity)
    {}

    ~lru_cache() noexcept = default;

    lru_cache(const lru_cache&) = default;
    lru_cache& operator=(const lru_cache&) = default;

    lru_cache(lru_cache&&) noexcept = default;
    lru_cache& operator=(lru_cache&&) noexcept = default;

    void put(const Key& key, const Value& value) {
        auto it = cache_.find(key);

        // if the key exists, then we update the value in it and mark it as MRU
        if (it != cache_.end()) {
            it->second->value = value;
            lines_.splice(lines_.begin(), lines_, it->second);
            return;
        }

        if (capacity_ == 0) {
            return;
        }

        // if the cache is full, remove the lru node from both the list and map
        if (cache_.size() == capacity_) {
            auto& lru = lines_.back();
            cache_.erase(lru.key);
            lines_.pop_back();
        }

        lines_.push_front(Node{key, value});
        cache_[key] = lines_.begin();
    }

    std::optional<Value> get(const Key& key) {
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return std::nullopt;
        }

        lines_.splice(lines_.begin(), lines_, it->second);
        return it->second->value;
    }

    bool contains(const Key& key) const noexcept {
        return cache_.contains(key);
    }

    bool erase(const Key& key) {
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return false;
        }

        lines_.erase(it->second);
        cache_.erase(it);
        return true;
    }

    std::size_t size() const noexcept {
        return cache_.size();
    } 

    std::size_t capacity() const noexcept {
        return capacity_;
    }
};

}  // namespace bmngxn
