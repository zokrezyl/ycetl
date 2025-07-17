#pragma once
#include <cstddef>

#include <ycetl/impl/multitype_handler.hpp>
#include <ycetl/trivial_shared_ptr.hpp>
#include <ycetl/type_system.hpp>

namespace ycetl {
namespace memory {

template <template <typename> class MemoryBackend, typename TypeSet>
class multitype_memory : public multitype_handler<MemoryBackend, TypeSet> {
public:
  using multitype_handler<MemoryBackend, TypeSet>::multitype_handler;
  using type_set = TypeSet;

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
