#pragma once

#include <atomic>
#include <optional>

namespace bmngxn {
    // item in the spsc must be default constructible and move constructible
    template<typename T>
    concept Queueable = std::default_initializable<T> && std::move_constructible<T>;

    /**
     * Lock-free SPSC queue  using a circular buffer: head as read index, tail as write index
     * 
     * the real cap is Capacity - 1 to distinguish between full and empty states
     */
    template<Queueable T, std::size_t N>
    class spsc_queue {
    private:
        static constexpr std::size_t Capacity = 1 << N;

        alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> head{0};
        alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> tail{0}; 
        alignas(std::hardware_destructive_interference_size) T buffer[Capacity]; 
    
    public:

        spsc_queue() = default;

        spsc_queue(const spsc_queue&) = delete;
        spsc_queue& operator=(const spsc_queue&) = delete;

        spsc_queue(spsc_queue&&) = delete;
        spsc_queue& operator=(spsc_queue&&) = delete;

        // old was enqueu(const T& item)
        //         buffer[current_tail] = std::move(item);
        // bug: we pass in a const ref and now were tryin to move it -> compiler falls back to copy assignments
        // 
        // -> bool enqueue(T item) -> pass by value and move
        // but passing T by value still involves constructing and destroying that tmp item param on the stack
        // perfect forwarding with variadic templates (?)
        // OR: lvalue/rvalue overloads (later)
        bool enqueue(T item) noexcept {
            std::size_t current_tail = tail.load(std::memory_order_relaxed); // producer is the only thread modifying tail -> no need to sync with itself
            std::size_t next_tail = (current_tail + 1) & (Capacity - 1);

            if (next_tail == head.load(std::memory_order_acquire)) return false; 

            buffer[current_tail] = std::move(item);
            tail.store(next_tail, std::memory_order_release);
            return true;
        }

        std::optional<T> dequeue() noexcept {
            std::size_t current_head = head.load(std::memory_order_relaxed); // sameeeeee

            if (current_head == tail.load(std::memory_order_acquire)) return std::nullopt; 

            T popped_item = std::move(buffer[current_head]); 
            head.store((current_head + 1) & (Capacity - 1), std::memory_order_release);
            return popped_item;
        }

        bool empty() const noexcept {
            return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
        }   

        bool full() const noexcept {
            std::size_t next_tail = (tail.load(std::memory_order_acquire) + 1) & (Capacity - 1);
            return next_tail == head.load(std::memory_order_acquire);
        }   

        std::size_t capacity() const noexcept {
            return Capacity - 1;
        }

    };

}