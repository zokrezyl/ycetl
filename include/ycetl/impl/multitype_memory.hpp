#pragma once
#include <cstddef>

#include <ycetl/impl/multitype_handler.hpp>
#include <ycetl/trivial_shared_ptr.hpp>
#include <ycetl/type_system.hpp>

namespace ycetl {
namespace memory {

#if 0
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

#endif

#if 0
template <template <typename> class MemoryBackend, typename... Ts>
constexpr auto make_shared_handlers(type_set<Ts...>) {
  return multitype_handler<
      trivial_shared_ptr,
      apply_wrapper_t<trivial_shared_ptr,
                      apply_wrapper_t<MemoryBackend, type_set<Ts...>>>>{
      trivial_shared_ptr<MemoryBackend<Ts>>(new MemoryBackend<Ts>{})...};
}
#endif

template <template <typename> class MemoryBackend, typename TypeSet>
class multitype_memory
    : public multitype_handler<
          trivial_shared_ptr,
          apply_wrapper_t<trivial_shared_ptr,
                          apply_wrapper_t<MemoryBackend, TypeSet>>> {
public:
  using type_set = TypeSet;

  template <typename T> constexpr T *allocate(std::size_t n) {
    return this->template get_handler<trivial_shared_ptr<MemoryBackend<T>>>()
        ->get()
        ->allocate(n);
  }

  template <typename T> constexpr void deallocate(T *p) {
    this->template get_handler<trivial_shared_ptr<MemoryBackend<T>>>()
        ->get()
        ->deallocate(p);
  }
};

} // namespace memory
} // namespace ycetl
