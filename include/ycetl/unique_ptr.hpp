#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace ycetl {

// Single-object move-only owner. No allocator parameter — uses
// new/delete, which are constexpr-callable since C++20 as long as the
// matching delete runs in the same constant-evaluation context.
//
// Array specialisation (T[]) and custom Deleter are intentionally out
// of scope until there's a real use case. The constructor/destructor
// pair is the load-bearing part; everything else mirrors std::unique_ptr.
template <typename T> class unique_ptr {
public:
  using element_type = T;
  using pointer = T *;

private:
  pointer _p = nullptr;

public:
  constexpr unique_ptr() noexcept = default;
  constexpr unique_ptr(std::nullptr_t) noexcept : _p(nullptr) {}
  constexpr explicit unique_ptr(pointer p) noexcept : _p(p) {}

  unique_ptr(const unique_ptr &) = delete;
  unique_ptr &operator=(const unique_ptr &) = delete;

  constexpr unique_ptr(unique_ptr &&o) noexcept : _p(o._p) { o._p = nullptr; }

  constexpr unique_ptr &operator=(unique_ptr &&o) noexcept {
    if (this != &o) {
      reset(o._p);
      o._p = nullptr;
    }
    return *this;
  }

  constexpr unique_ptr &operator=(std::nullptr_t) noexcept {
    reset();
    return *this;
  }

  constexpr ~unique_ptr() { reset(); }

  constexpr pointer get() const noexcept { return _p; }
  constexpr explicit operator bool() const noexcept { return _p != nullptr; }

  constexpr T &operator*() const noexcept { return *_p; }
  constexpr pointer operator->() const noexcept { return _p; }

  constexpr pointer release() noexcept {
    pointer p = _p;
    _p = nullptr;
    return p;
  }

  // Order: take ownership of `p` first, then delete the old object —
  // matches std::unique_ptr::reset and is robust if `p` somehow alias-
  // points into the previous object's substructure.
  constexpr void reset(pointer p = nullptr) noexcept {
    pointer old = _p;
    _p = p;
    if (old)
      delete old;
  }

  constexpr void swap(unique_ptr &o) noexcept {
    pointer tmp = _p;
    _p = o._p;
    o._p = tmp;
  }

  friend constexpr bool operator==(const unique_ptr &a,
                                   std::nullptr_t) noexcept {
    return a._p == nullptr;
  }
  friend constexpr bool operator==(const unique_ptr &a,
                                   const unique_ptr &b) noexcept {
    return a._p == b._p;
  }
};

template <typename T, typename... Args>
constexpr unique_ptr<T> make_unique(Args &&...args) {
  return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

} // namespace ycetl
