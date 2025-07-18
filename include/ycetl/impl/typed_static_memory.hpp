#pragma once
#include <array>
#include <cstddef>
#include <stdexcept>

namespace ycetl {

constexpr std::size_t typed_static_memory_size = 1024;

template <typename T> class typed_static_memory {
private:
  std::array<T, typed_static_memory_size> _storage;
  T *_current_ptr;
  std::size_t _remaining_size;

public:
  constexpr typed_static_memory() noexcept
      : _storage{}, _current_ptr(_storage.data()),
        _remaining_size(typed_static_memory_size) {}

  constexpr ~typed_static_memory() noexcept = default;

  constexpr T *allocate(std::size_t size) {
    if (size == 0) {
      return nullptr;
    }

    if (size > _remaining_size) {
      return nullptr;
    }

    T *allocated_ptr = _current_ptr;
    _current_ptr += size;
    _remaining_size -= size;
    return allocated_ptr;
  }

  constexpr void deallocate(T *ptr) noexcept { (void)ptr; }
  constexpr void deallocate(T *ptr, std::size_t size) noexcept {
    (void)ptr;
    (void)size;
  }
};

} // namespace ycetl
