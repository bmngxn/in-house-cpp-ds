#pragma once

#include <atomic>
#include <cstddef>
#include <utility>

namespace bmngxn {

/**
 * The standard shifts the Deleter type responsibility away from the shared_ptr
 * class and onto the polymorphic control block base class.
 */

template <typename T>
struct default_deleter {
    void operator()(T* ptr) const { 
        delete ptr;
    }
};

/** 
 * The Abstract Base Control Block
 * 
 * The shared_ptr only interacts with this base class and knows nothing about
 * T or the Deleter
 */ 
struct control_block_base {
    std::atomic<std::size_t> ref_count{1};

    virtual ~control_block_base() = default;

    virtual void dispose() noexcept = 0; // Destroys the mana   ged object
    virtual void destroy() noexcept = 0; // Deallocates the control block itself
};

/** 
 * The Derived Control Block for standard pointers
 * 
 * This moves the Deleter type into the heap allocation, hiding it from 
 * shared_ptr.
 * 
 */
template <typename Ptr, typename Deleter>
struct control_block_ptr : public control_block_base {
    Ptr ptr_;
    Deleter deleter_;

    control_block_ptr(Ptr ptr, Deleter deleter) noexcept
        : ptr_(ptr)
        , deleter_(std::move(deleter)) 
    {}

    void dispose() noexcept override {
        deleter_(ptr_);
    }

    void destroy() noexcept override {
        delete this;
    }
};

template <typename T>
class shared_ptr {
private:
    T* ptr_;
    control_block_base* control_;

    void acquire() noexcept {
        if (control_ != nullptr) {
            control_->ref_count.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void release() noexcept {
        if (control_ == nullptr) {
            return;
        }

        control_block_base* old_control = control_;
        ptr_ = nullptr;
        control_ = nullptr;

        // when the last shared_ptr dies, we pass  destruction to the virt funcs
        if (old_control->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            old_control->dispose(); 
            old_control->destroy();
        }
    }

public:
    shared_ptr() noexcept
        : ptr_(nullptr)
        , control_(nullptr)
    {}

    shared_ptr(std::nullptr_t) noexcept
        : ptr_(nullptr)
        , control_(nullptr)
    {}

    // constructor can accept an optional custom deleter
    template <typename Deleter = default_deleter<T>>
    explicit shared_ptr(T* ptr, Deleter deleter = Deleter())
        : ptr_(ptr)
        , control_(ptr == nullptr ? nullptr : new control_block_ptr<T*, Deleter>(ptr, std::move(deleter)))
    {}

    ~shared_ptr() {
        release();
    }

    shared_ptr(const shared_ptr& other) noexcept
        : ptr_(other.ptr_)
        , control_(other.control_)
    {
        acquire();
    }

    shared_ptr& operator=(const shared_ptr& other) noexcept {
        if (this != &other) {
            release();
            ptr_ = other.ptr_;
            control_ = other.control_;
            acquire();
        }
        return *this;
    }

    shared_ptr(shared_ptr&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr))
        , control_(std::exchange(other.control_, nullptr))
    {}

    shared_ptr& operator=(shared_ptr&& other) noexcept {
        if (this != &other) {
            release();
            ptr_ = std::exchange(other.ptr_, nullptr);
            control_ = std::exchange(other.control_, nullptr);
        }
        return *this;
    }

    shared_ptr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    T* get() const noexcept {
        return ptr_;
    }

    T& operator*() const noexcept {
        return *ptr_;
    }

    T* operator->() const noexcept {
        return ptr_;
    }

    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    std::size_t use_count() const noexcept {
        return control_ == nullptr ? 0 : control_->ref_count.load(std::memory_order_acquire);
    }

    bool unique() const noexcept {
        return use_count() == 1;
    }

    template <typename Deleter = default_deleter<T>>
    void reset(T* ptr = nullptr, Deleter deleter = Deleter()) {
        if (ptr_ == ptr) return;

        release();

        if (ptr != nullptr) {
            control_ = new control_block_ptr<T*, Deleter>(ptr, std::move(deleter));
            ptr_ = ptr;
        }
    }

    void swap(shared_ptr& other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(control_, other.control_);
    }

    friend bool operator==(const shared_ptr& x, std::nullptr_t) noexcept { 
        return x.get() == nullptr; 
    }

    friend bool operator!=(const shared_ptr& x, std::nullptr_t) noexcept { 
        return x.get() != nullptr; 
    }

    friend bool operator==(std::nullptr_t, const shared_ptr& x) noexcept { 
        return x.get() == nullptr; 
    }

    friend bool operator!=(std::nullptr_t, const shared_ptr& x) noexcept { 
        return x.get() != nullptr; 
    }

    friend void swap(shared_ptr& lhs, shared_ptr& rhs) noexcept {
        lhs.swap(rhs);
    }
};

}