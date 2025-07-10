#pragma once

#include <ycetl/dynamic_storage.hpp>

namespace ycetl {

namespace memory {

template <typename T, typename StorageType = ycetl::memory::dynamic_storage<T>>
struct allocator {
  StorageType _storage;

public:
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using propagate_on_container_move_assignment = std::true_type;
  using is_always_equal = std::true_type;

  constexpr allocator() : _storage() {}

  constexpr T *allocate(std::size_t n) { return _storage.allocate(n); }

  constexpr void deallocate(T *ptr, std::size_t n) {
    if (n == 0 || ptr == nullptr) {
      return;
    }
    _storage.deallocate(ptr);
  }
};

} // namespace memory
} // namespace ycetl
