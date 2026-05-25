// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>

#include <ycetl/shared_ptr.hpp>

namespace ycetl {

// Non-owning observer of a shared_ptr-managed object. Holds the same
// control block as the shared_ptr it was constructed from, but bumps
// only weak_count_ — so it keeps the control block alive without
// keeping the managed object alive. lock() returns a strong shared_ptr
// if the object is still around, an empty one if it has been destroyed.
//
// Thread-unsafe by design — same trade as ycetl::shared_ptr. Don't
// share a weak_ptr across threads without external synchronisation.
template <typename T> class weak_ptr {
private:
  _shared_ptr_control_block *control_block_ = nullptr;
  T *ptr_ = nullptr;

  constexpr void release_weak() noexcept {
    if (!control_block_)
      return;
    if (--control_block_->weak_count_ == 0) {
      delete control_block_;
    }
    control_block_ = nullptr;
    ptr_ = nullptr;
  }

public:
  constexpr weak_ptr() noexcept = default;

  // Construct from a shared_ptr — pin its control block (++weak).
  constexpr weak_ptr(const shared_ptr<T> &s) noexcept
      : control_block_(s.control_block_), ptr_(s.ptr_) {
    if (control_block_)
      ++control_block_->weak_count_;
  }

  constexpr weak_ptr(const weak_ptr &o) noexcept
      : control_block_(o.control_block_), ptr_(o.ptr_) {
    if (control_block_)
      ++control_block_->weak_count_;
  }

  constexpr weak_ptr(weak_ptr &&o) noexcept
      : control_block_(o.control_block_), ptr_(o.ptr_) {
    o.control_block_ = nullptr;
    o.ptr_ = nullptr;
  }

  constexpr weak_ptr &operator=(const weak_ptr &o) noexcept {
    if (this != &o) {
      release_weak();
      control_block_ = o.control_block_;
      ptr_ = o.ptr_;
      if (control_block_)
        ++control_block_->weak_count_;
    }
    return *this;
  }

  constexpr weak_ptr &operator=(weak_ptr &&o) noexcept {
    if (this != &o) {
      release_weak();
      control_block_ = o.control_block_;
      ptr_ = o.ptr_;
      o.control_block_ = nullptr;
      o.ptr_ = nullptr;
    }
    return *this;
  }

  constexpr weak_ptr &operator=(const shared_ptr<T> &s) noexcept {
    release_weak();
    control_block_ = s.control_block_;
    ptr_ = s.ptr_;
    if (control_block_)
      ++control_block_->weak_count_;
    return *this;
  }

  constexpr ~weak_ptr() noexcept { release_weak(); }

  constexpr std::size_t use_count() const noexcept {
    return control_block_ ? control_block_->ref_count_ : 0;
  }
  constexpr bool expired() const noexcept { return use_count() == 0; }

  constexpr void reset() noexcept { release_weak(); }

  // Recover a strong owner if the object is still alive. Empty
  // shared_ptr if not.
  constexpr shared_ptr<T> lock() const noexcept {
    if (!control_block_ || control_block_->ref_count_ == 0)
      return shared_ptr<T>{};
    return shared_ptr<T>(control_block_, ptr_, /*tag=*/0);
  }
};

} // namespace ycetl
