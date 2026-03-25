#pragma once

#include <vector>
#include <mutex>

namespace bmngxn {

template <typename Key, typename Value>
class hash_map {
private:
    enum class State {
        Empty,
        Occupied,
        Deleted,
    };

    struct Entry {
        Key key;
        Value value;
        State state = State::Empty;
    };  

    std::vector<Entry> table_;
    std::size_t size_;
    std::size_t capacity_ = 16;

    static constexpr npos = std::numeric_limits<std::size_t>::max();

    static constexpr double MAX_LOAD_FACTOR = 0.5;

    void resize() {
        if (static_cast<double>(size_ + 1) / capacity_ <= MAX_LOAD_FACTOR) return;

        if (capacity_ * 2 >= std::numeric_limits<std::size_t>::max()) {
            throw std::overflow_error("Capaacity overflow");
        }

        resize_and_rehash(capacity_ * 2);
    }

    void resize_and_rehash(std::size_t new_capacity) {
        std::vector<Entry> old_table = std::move(table_); //calls move constructor of std::vector

        table_.resize(new_capacity);
        size_ = 0;
        capacity_ = new_capacity;

        for (const auto& entry : old_table) {
            if (entry.state == State::Occupied) {
                insert(entry.key, entry.value);
            }
        }
    }

    std::size_t hash(const Key& key) const {
        return std::hash<Key>{}(key);
    }

    
    std::size_t find_index(const Key& key) const {
        std::size_t index = hash(key);

        for (std::size_t i = 0; i < capacity_; i++) {
            std::size_t probe = (index + i) % capacity_;
            const auto& entry = table_[probe];

            if (entry.state == State::Empty) {
                return npos;
            }

            if (entry.state == State::Occupied && entry.key == key) {
                return probe;
            }
        }

        return npos;
    }

public:
    hash_map() 
        : size_(0)
    {
        table_.resize(capacity_);
    };


    hash_map(const hash_map&) = default;
    hash_map& operator=(const hash_map&) = default;

    hash_map(hash_map&&) noexcept = default;
    hash_map& operator=(hash_map&&) noexcept = default;

    void insert(const Key& key, const Value& value) {
        resize();

        std::size_t index = hash(key);

        // first DELETED idx found is the priority slot to insert into compare to EMPTY
        std::optional<std::size_t> first_deleted;

        for (std::size_t i = 0; i < capacity_; i++) {
            std::size_t probe = (index + i) % capacity_;
            auto& entry = table_[probe];
            if (entry.state == State::Occupied) {
                if (entry.key == key) {
                    entry.value = value;
                    return;
                }
            } else if (entry.state == State::Deleted) {
                if (!first_deleted.has_value()) first_deleted = probe;
            } else {
                std::size_t slot = first_deleted.value_or(probe);
                table_[slot] = {key, value, State::Occupied};
                size_++;
                return;
            }
        }
    }

    bool erase(const Key& key) {

        auto index = find_index(key);
        if (!index == npos) return false;

        table_[*index].state = State::Deleted;
        size_--;
        return true;
    }

    Value& get(const Key& key) {
        auto index = find_index(key);

        if (!index == npos) throw std::out_of_range("Key not found");

        return table_[*index].value;
    }

    const Value& get(const Key& key) const {
        auto index = find_index(key);

        if (!index == npos) throw std::out_of_range("Key not found");

        return table_[*index].value;
    }

    bool contains(const Key& key) const {
        return find_index(key) != npos;
    }

    std::size_t get_size() const {
        return size_;
    }

    std::size_t get_capacity() const {
        return capacity_;
    }
    
};



}