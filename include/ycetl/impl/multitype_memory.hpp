#pragma once
#include <cstddef>

#include <ycetl/impl/multitype_handler.hpp>
#include <ycetl/trivial_shared_ptr.hpp>
#include <ycetl/type_system.hpp>

namespace ycetl {
namespace memory {

#if 0
template <template <typename> class MemoryBackend, typename TypeSet>
class multitype_memory_unwrapped
    : public multitype_handler<MemoryBackend, TypeSet> {
public:
  using multitype_handler_unwrapped<MemoryBackend, TypeSet>::multitype_handler;
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
#endif

template <template <typename> class MemoryBackend, typename TypeSet>
class multitype_memory
    : public multitype_handler<
          trivial_shared_ptr,
          apply_wrapper_t<trivial_shared_ptr,
                          apply_wrapper_t<MemoryBackend, TypeSet>>> {
public:
  using handler_type = multitype_handler<
      trivial_shared_ptr,
      apply_wrapper_t<trivial_shared_ptr,
                      apply_wrapper_t<MemoryBackend, TypeSet>>>;

  using type_set = TypeSet;

  constexpr multitype_memory() : handler_type{} {}

  // Construct by sharing handlers from an existing multitype_memory
  constexpr multitype_memory(const handler_type &shared_handlers)
      : handler_type(shared_handlers) {}

  template <typename T> constexpr T *allocate(std::size_t n) {
    return this->template get_handler<trivial_shared_ptr<MemoryBackend<T>>>()
        ->allocate(n);
  }

  template <typename T> constexpr void deallocate(T *p) {
    this->template get_handler<trivial_shared_ptr<MemoryBackend<T>>>()
        ->deallocate(p);
  }
};

} // namespace memory
} // namespace ycetl
