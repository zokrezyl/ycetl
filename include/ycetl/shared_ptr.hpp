#pragma once

#include <cstddef>

namespace ycetl {

template <typename T> class weak_ptr; // fwd

// Two-counter control block: strong refs (managed object lifetime) and
// weak refs (control block lifetime). The strong refs share a single
// "implicit weak" reference — i.e. weak_count_ starts at 1 alongside
// the first strong, and gets decremented exactly once when the last
// strong goes away. This is the libstdc++ shape and the only way for
// weak_ptr::lock() to work safely.
struct _shared_ptr_control_block {
    std::size_t ref_count_;   // strong refs
    std::size_t weak_count_;  // weak refs + (1 if any strong refs)
};

// Reference-counted owner. Thread-unsafe by design — same as the
// existing ycetl::trivial_shared_ptr, suitable for constexpr / single-
// threaded runtime workloads.
template <typename T> class shared_ptr {
private:
    _shared_ptr_control_block *control_block_;
    T                         *ptr_;

    // weak_ptr needs raw access to the same control_block + ptr to
    // construct a shared from a weak and to bump weak_count_.
    friend class weak_ptr<T>;

    // Internal: release a strong reference. Destroys the managed
    // object when ref_count_ → 0, and the control block itself when
    // weak_count_ → 0 (which happens immediately if no weak_ptr is
    // observing).
    constexpr void release_strong() noexcept {
        if (!control_block_) return;
        if (--control_block_->ref_count_ == 0) {
            delete ptr_;
            ptr_ = nullptr;
            if (--control_block_->weak_count_ == 0) {
                delete control_block_;
            }
        }
        control_block_ = nullptr;
    }

    // Used by weak_ptr::lock() to construct a shared from raw pieces
    // without re-allocating the control block.
    constexpr shared_ptr(_shared_ptr_control_block *cb, T *p,
                         int /*tag — disambiguates from public(T*)*/) noexcept
        : control_block_(cb), ptr_(p)
    {
        if (control_block_) ++control_block_->ref_count_;
    }

public:
    constexpr shared_ptr() noexcept : control_block_(nullptr), ptr_(nullptr) {}

    constexpr explicit shared_ptr(T *p) : control_block_(nullptr), ptr_(p) {
        if (p) control_block_ = new _shared_ptr_control_block{1, 1};
    }

    constexpr shared_ptr(const shared_ptr &other) noexcept
        : control_block_(other.control_block_), ptr_(other.ptr_)
    {
        if (control_block_) ++control_block_->ref_count_;
    }

    constexpr shared_ptr(shared_ptr &&other) noexcept
        : control_block_(other.control_block_), ptr_(other.ptr_)
    {
        other.control_block_ = nullptr;
        other.ptr_           = nullptr;
    }

    constexpr shared_ptr &operator=(const shared_ptr &other) noexcept {
        if (this != &other) {
            release_strong();
            control_block_ = other.control_block_;
            ptr_           = other.ptr_;
            if (control_block_) ++control_block_->ref_count_;
        }
        return *this;
    }

    constexpr shared_ptr &operator=(shared_ptr &&other) noexcept {
        if (this != &other) {
            release_strong();
            control_block_       = other.control_block_;
            ptr_                 = other.ptr_;
            other.control_block_ = nullptr;
            other.ptr_           = nullptr;
        }
        return *this;
    }

    constexpr ~shared_ptr() noexcept { release_strong(); }

    constexpr T &operator*()  const noexcept { return *ptr_; }
    constexpr T *operator->() const noexcept { return ptr_; }
    constexpr T *get()        const noexcept { return ptr_; }
    constexpr std::size_t use_count() const noexcept {
        return control_block_ ? control_block_->ref_count_ : 0;
    }
    constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }
};

// CTAD: same hook the existing implementation exposed.
template <typename D> shared_ptr(D *) -> shared_ptr<typename D::object_type>;

} // namespace ycetl
