#pragma once

#include <cstddef>

namespace ycetl {

// Control block for reference counting
// (unchanged)
struct _shared_ptr_control_block {
  std::size_t ref_count_; // Reference count for shared ownership
};

// Minimal shared_ptr implementation
template <typename T> class shared_ptr {
private:
  _shared_ptr_control_block *control_block_;
  T *ptr_;

public:
  // Default constructor: empty
  constexpr shared_ptr() noexcept : control_block_(nullptr), ptr_(nullptr) {}

  // Construct from raw pointer
  constexpr explicit shared_ptr(T *p) : control_block_(nullptr), ptr_(p) {
    if (p) {
      control_block_ = new _shared_ptr_control_block{1};
    }
  }

  // Copy constructor
  constexpr shared_ptr(const shared_ptr &other) noexcept
      : control_block_(other.control_block_), ptr_(other.ptr_) {
    if (control_block_) {
      ++control_block_->ref_count_;
    }
  }

  // Move constructor
  constexpr shared_ptr(shared_ptr &&other) noexcept
      : control_block_(other.control_block_), ptr_(other.ptr_) {
    other.control_block_ = nullptr;
    other.ptr_ = nullptr;
  }

  // Copy assignment
  constexpr shared_ptr &operator=(const shared_ptr &other) noexcept {
    if (this != &other) {
      // release current
      if (control_block_) {
        if (--control_block_->ref_count_ == 0) {
          delete ptr_;
          delete control_block_;
        }
      }
      // copy from other
      control_block_ = other.control_block_;
      ptr_ = other.ptr_;
      if (control_block_) {
        ++control_block_->ref_count_;
      }
    }
    return *this;
  }

  // Move assignment
  constexpr shared_ptr &operator=(shared_ptr &&other) noexcept {
    if (this != &other) {
      // release current
      if (control_block_) {
        if (--control_block_->ref_count_ == 0) {
          delete ptr_;
          delete control_block_;
        }
      }
      // move from other
      control_block_ = other.control_block_;
      ptr_ = other.ptr_;
      other.control_block_ = nullptr;
      other.ptr_ = nullptr;
    }
    return *this;
  }

  // Destructor
  constexpr ~shared_ptr() noexcept {
    if (control_block_) {
      if (--control_block_->ref_count_ == 0) {
        delete ptr_;
        delete control_block_;
      }
    }
  }

  // Observers
  constexpr T &operator*() const noexcept { return *ptr_; }
  constexpr T *operator->() const noexcept { return ptr_; }
  constexpr T *get() const noexcept { return ptr_; }
  constexpr std::size_t use_count() const noexcept {
    return control_block_ ? control_block_->ref_count_ : 0;
  }
  constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }
};

// CTAD guide: deduce from any D* where D defines object_type
template <typename D> shared_ptr(D *) -> shared_ptr<typename D::object_type>;

} // namespace ycetl
