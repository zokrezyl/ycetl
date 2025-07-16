#pragma once
#include <cstddef>

namespace ycetl {

template <typename T> class trivial_shared_ptr {
  T *ptr{};
  std::size_t *count{};

public:
  constexpr trivial_shared_ptr() : ptr(new T{}), count(new std::size_t(1)) {}

  constexpr explicit trivial_shared_ptr(T *p)
      : ptr(p), count(new std::size_t(1)) {}

  constexpr trivial_shared_ptr(const trivial_shared_ptr &other)
      : ptr(other.ptr), count(other.count) {
    if (count)
      ++(*count);
  }

  constexpr trivial_shared_ptr &operator=(const trivial_shared_ptr &other) {
    if (this != &other) {
      reset();
      ptr = other.ptr;
      count = other.count;
      if (count)
        ++(*count);
    }
    return *this;
  }

  constexpr ~trivial_shared_ptr() { reset(); }

  constexpr void reset() {
    if (count && --(*count) == 0) {
      delete ptr;
      delete count;
    }
    ptr = nullptr;
    count = nullptr;
  }

  constexpr T *get() const { return ptr; }
  constexpr T &operator*() const { return *ptr; }
  constexpr T *operator->() const { return ptr; }
};

} // namespace ycetl
