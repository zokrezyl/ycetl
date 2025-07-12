#pragma once
#include <cstddef>

#include <ycetl/impl/multitype.hpp>

namespace ycetl {
namespace memory {

template <template <typename> class StorageBackend, typename TypeSet>
class multitype_allocator : public multitype_handler<StorageBackend, TypeSet> {
public:
  using multitype_handler<StorageBackend, TypeSet>::multitype_handler;

  // Array allocation
  template <typename T> constexpr T *allocate(std::size_t n) {
    return this->template get_handler<T>().allocate(n);
  }
  // Single object deallocation
  template <typename T> constexpr void deallocate(T *p) {
    this->template get_handler<T>().deallocate(p);
  }
};

} // namespace memory
} // namespace ycetl
