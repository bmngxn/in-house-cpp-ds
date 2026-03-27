#pragma once

#include "hash_map/hash_map.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace bmngxn {

template <typename Key, typename Value>
class lru_cache {
private:
    static constexpr int NULL_INDEX = -1;

    struct Node {
        Key key;
        Value value;

        int prev = NULL_INDEX;
        int next = NULL_INDEX;

        Node() = default;

        Node(Key k, Value v)
            : key(std::move(k))
            , value(std::move(v))
        {}
    };

    std::size_t capacity_;
    std::vector<Node> lines_;

    hash_map<Key, int> cache_;

    int head_ = NULL_INDEX;   
    int tail_ = NULL_INDEX;     
    int free_head_ = NULL_INDEX;

    void detach(int index) {
        int prev = lines_[index].prev;
        int next = lines_[index].next;

        if (prev != NULL_INDEX) {
            lines_[prev].next = next;
        } else {
            head_ = next;
        }

        if (next != NULL_INDEX) {
            lines_[next].prev = prev;
        } else {
            tail_ = prev; 
        }
    }

    void push_front(int index) {
        lines_[index].prev = NULL_INDEX;
        lines_[index].next = head_;

        if (head_ != NULL_INDEX) {
            lines_[head_].prev = index;
        }

        head_ = index;

        if (tail_ == NULL_INDEX) {
            tail_ = index;
        }
    }

public:
    explicit lru_cache(std::size_t capacity) noexcept
        : capacity_(capacity)
    {
        if (capacity_ == 0) return;
        
        lines_.resize(capacity_);

        cache_.reserve(capacity_);

        for (int i = 0; i < static_cast<int>(capacity_) - 1; i++) {
            lines_[i].next = i + 1;
        }

        lines_[capacity_ - 1].next = NULL_INDEX;
        free_head_ = 0;
    }

    ~lru_cache() noexcept = default;

    lru_cache(const lru_cache&) = default;
    lru_cache& operator=(const lru_cache&) = default;

    lru_cache(lru_cache&&) noexcept = default;
    lru_cache& operator=(lru_cache&&) noexcept = default;

    void put(const Key& key, const Value& value) {
        if (capacity_ == 0) return;

        int* idx_ptr = cache_.get(key);
        
        if (idx_ptr != nullptr) {
            int index = *idx_ptr;
            lines_[index].value = value;
            detach(index);
            push_front(index);
            return;
        }

        int new_index = NULL_INDEX;

        if (cache_.get_size() == capacity_) {
            new_index = tail_;
            cache_.erase(lines_[new_index].key);
            detach(new_index);
        } else {
            new_index = free_head_;
            free_head_ = lines_[free_head_].next;
        }

        lines_[new_index].key = key;
        lines_[new_index].value = value;
        push_front(new_index);
        
    
        cache_.insert(key, new_index);
    }

    Value* get(const Key& key) {
        int* idx_ptr = cache_.get(key);
        if (idx_ptr == nullptr) return nullptr;

        int index = *idx_ptr;
        detach(index);
        push_front(index);
        
        return &lines_[index].value;
    }

    bool contains(const Key& key) const noexcept {
        return cache_.contains(key);
    }

    bool erase(const Key& key) {
        int* idx_ptr = cache_.get(key);
        if (idx_ptr == nullptr) return false;

        int index = *idx_ptr;
        cache_.erase(key);
        detach(index);

        lines_[index].next = free_head_;
        free_head_ = index;

        return true;
    }

    std::size_t size() const noexcept {
        return cache_.get_size();
    } 

    std::size_t capacity() const noexcept {
        return capacity_;
    }
};

} 