#pragma once

#include <ycetl/multitype.hpp>
// clang-format off
namespace ycetl {
namespace memory {

template <template <typename> class StorageBackend, typename... Types>
class multitype_storage : public multitype_handler<StorageBackend, type_set<Types...>> {
public:
  using multitype_handler<StorageBackend, type_set<Types...>>::multitype_handler;

  // Single object allocation
  template <typename T>
  constexpr T* allocate() {
    return this->template get_handler<T>().allocate();
  }

  // Array allocation
  template <typename T>
  constexpr T* allocate(std::size_t n) {
    return this->template get_handler<T>().allocate(n);
  }

  // Single object deallocation
  template <typename T>
  constexpr void deallocate(T* p) {
    this->template get_handler<T>().deallocate(p);
  }

  // Array deallocation
  template <typename T>
  constexpr void deallocate(T* p, std::size_t n) {
    this->template get_handler<T>().deallocate(p, n);
  }
};

}
}

// clang-format on
