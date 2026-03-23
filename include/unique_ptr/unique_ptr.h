#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace bmngxn {

template <typename T>
struct default_deleter {
    void operator()(T* ptr) const {
        delete ptr;
    }
};

template <typename T>
struct default_deleter<T[]> {
    void operator()(T* ptr) const {
        delete[] ptr;
    }
};

/**
 * - A smart pointer that owns and manages another object through a pointer 
 * and disposes of that object when the unique_ptr goes out of scope
 * * - Includes a default deleter, move-only ownership semantics
 * * - Supported operators: get/release/reset/swap
 * 
 * - Design choices:
 *   +) unique_ptr inherits from Deleter and access it through:
 *          static_cast<const Deleter&>(*this)
 *   -> no deleter as member function 
 *   -> unique_ptr stays 8 bytes
 * 
 *   +) "hidden friend" idiom: instead of globally define the operations (==, !=, ...)
 *   in the global namespace (::bmngxn), which forces the compiler to look at this
 *   template every time we call an operation in the namespace, inheritly slows down
 *   compilation
 *   -> we define it inside the class as a friend -> no longer a template. this
 *   also enforces Argument-Dependent Lookup (ADL) Safety: the swap or operator== 
 *   function is completely invisible to the rest of the codebase unless one of 
 *   the arguments explicitly passed to it is a unique_ptr
 */
template <typename T, typename Deleter = default_deleter<T>>
class unique_ptr : public Deleter {
public:
    unique_ptr() noexcept 
        : Deleter()
        , ptr_(nullptr) 
    {}

    unique_ptr(std::nullptr_t) noexcept 
        : Deleter()
        , ptr_(nullptr) 
    {}

    explicit unique_ptr(T* ptr) noexcept 
        : Deleter()
        , ptr_(ptr) 
    {}

    unique_ptr(T* ptr, const Deleter& deleter) noexcept
        : Deleter(deleter)
        , ptr_(ptr) 
    {}

    unique_ptr(T* ptr, Deleter&& deleter) noexcept
        : Deleter(std::move(deleter))
        , ptr_(ptr) 
    {}

    // copy semantics are not allowed
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;

    unique_ptr(unique_ptr&& other) noexcept
        : Deleter(std::move(other.get_deleter()))
        , ptr_(std::exchange(other.ptr_, nullptr)) 
    {}

    // transfer ownership with move semantics
    unique_ptr& operator=(unique_ptr&& other) noexcept {
        if (this != &other) {
            reset(std::exchange(other.ptr_, nullptr));
            get_deleter() = std::move(other.get_deleter());
        }
        return *this;
    }

    // destroy owned object
    unique_ptr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    ~unique_ptr() {
        if (ptr_ != nullptr) {
            get_deleter()(ptr_);
        }
    }

    T* get() const noexcept {
        return ptr_;
    }

    Deleter& get_deleter() noexcept {
        return static_cast<Deleter&>(*this);
    }

    const Deleter& get_deleter() const noexcept {
        return static_cast<const Deleter&>(*this);
    }

    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    T& operator*() const noexcept {
        return *ptr_;
    }

    T* operator->() const noexcept {
        return ptr_;
    }

    T* release() noexcept {
        T* released = ptr_;
        ptr_ = nullptr;
        return released;
    }

    void reset(T* ptr = nullptr) noexcept {
        if (ptr_ == ptr) return;

        T* old_ptr = ptr_;
        ptr_ = ptr;

        if (old_ptr != nullptr) {
            get_deleter()(old_ptr);
        }
    }

    void swap(unique_ptr& other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(get_deleter(), other.get_deleter());
    }

    friend bool operator==(const unique_ptr& x, std::nullptr_t) noexcept {
        return x.get() == nullptr;
    }

    friend bool operator!=(const unique_ptr& x, std::nullptr_t) noexcept {
        return x.get() != nullptr;
    }

    friend bool operator==(std::nullptr_t, const unique_ptr& x) noexcept {
        return x.get() == nullptr;
    }

    friend bool operator!=(std::nullptr_t, const unique_ptr& x) noexcept {
        return x.get() != nullptr;
    }

    friend void swap(unique_ptr& lhs, unique_ptr& rhs) noexcept {
        lhs.swap(rhs);
    }

private:
    T* ptr_;
};



template <typename T, typename Deleter>
class unique_ptr<T[], Deleter> : public Deleter {
public:
    unique_ptr() noexcept 
        : Deleter()
        , ptr_(nullptr) 
    {}

    unique_ptr(std::nullptr_t) noexcept 
        : Deleter()
        , ptr_(nullptr) 
    {}

    explicit unique_ptr(T* ptr) noexcept 
        : Deleter()
        , ptr_(ptr) 
    {}

    unique_ptr(T* ptr, const Deleter& deleter) noexcept
        : Deleter(deleter)
        , ptr_(ptr) 
    {}

    unique_ptr(T* ptr, Deleter&& deleter) noexcept
        : Deleter(std::move(deleter))
        , ptr_(ptr) 
    {}

    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;

    unique_ptr(unique_ptr&& other) noexcept
        : Deleter(std::move(other.get_deleter()))
        , ptr_(std::exchange(other.ptr_, nullptr))
    {}

    unique_ptr& operator=(unique_ptr&& other) noexcept {
        if (this != &other) {
            reset(std::exchange(other.ptr_, nullptr));
            get_deleter() = std::move(other.get_deleter());
        }
        return *this;
    }

    unique_ptr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    ~unique_ptr() {
        if (ptr_ != nullptr) {
            get_deleter()(ptr_);
        }
    }

    T* get() const noexcept {
        return ptr_;
    }

    Deleter& get_deleter() noexcept {
        return static_cast<Deleter&>(*this);
    }

    const Deleter& get_deleter() const noexcept {
        return static_cast<const Deleter&>(*this);
    }

    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    T& operator[](std::size_t index) const noexcept {
        return ptr_[index];
    }

    T* release() noexcept {
        T* released = ptr_;
        ptr_ = nullptr;
        return released;
    }

    void reset(T* ptr = nullptr) noexcept {
        if (ptr_ == ptr) {
            return;
        }

        T* old_ptr = ptr_;
        ptr_ = ptr;

        if (old_ptr != nullptr) {
            get_deleter()(old_ptr);
        }
    }

    void swap(unique_ptr& other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(get_deleter(), other.get_deleter());
    }

    friend bool operator==(const unique_ptr& x, std::nullptr_t) noexcept {
        return x.get() == nullptr;
    }

    friend bool operator!=(const unique_ptr& x, std::nullptr_t) noexcept {
        return x.get() != nullptr;
    }

    friend bool operator==(std::nullptr_t, const unique_ptr& x) noexcept {
        return x.get() == nullptr;
    }

    friend bool operator!=(std::nullptr_t, const unique_ptr& x) noexcept {
        return x.get() != nullptr;
    }

    friend void swap(unique_ptr& lhs, unique_ptr& rhs) noexcept {
        lhs.swap(rhs);
    }

private:
    T* ptr_;  
};

}  // namespace bmngxn