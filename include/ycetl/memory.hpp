#pragma once
#include <bit>
// #include <cstring>
#include <memory>
// #include <ycetl/allocator_traits.hpp>
#include <ycetl/multitype_memory.hpp>
#include <ycetl/trivial_shared_ptr.hpp>
#include <ycetl/typed_dynamic_memory.hpp>
#include <ycetl/typed_static_memory.hpp>
#include <ycetl/types.hpp>

namespace ycetl {

template <typename T, typename MultitypeMemory>
constexpr auto allocate(MultitypeMemory &alloc, std::size_t n) {
  return alloc.template allocate<T>(n);
}

template <typename T, typename MultitypeMemory>
constexpr void deallocate(MultitypeMemory &alloc,
                          pointer_type_t<T, MultitypeMemory> ptr,
                          std::size_t n) {
  alloc.template deallocate<T>(ptr, n);
}

template <typename PointerType, typename... Args>
constexpr PointerType construct_at(PointerType ptr, Args &&...args) {
  return std::construct_at(ptr.get(), std::forward<Args>(args)...);
}

template <typename PointerType> constexpr void destroy_at(PointerType ptr) {
  std::destroy_at(ptr.get());
}

template <typename Backend, typename... Args>
constexpr static_synthetic_pointer<Backend>
construct_at(static_synthetic_pointer<Backend> ptr, Args &&...args) {
  std::construct_at(ptr.get(), std::forward<Args>(args)...);
  return ptr;
}

template <typename Backend, typename... Args>
constexpr dynamic_synthetic_pointer<Backend>
construct_at(dynamic_synthetic_pointer<Backend> ptr, Args &&...args) {
  std::construct_at(ptr.get(), std::forward<Args>(args)...);
  return ptr;
}

// helper function to allocate and construct an object of type array of T
template <typename T, typename MultitypeMemory, typename... Args>
constexpr auto allocate_and_construct_n(MultitypeMemory &alloc, std::size_t n,
                                        Args &&...args) {
  auto ptr = allocate<T>(alloc, n);
  for (std::size_t i = 0; i < n; ++i) {
    construct_at(ptr + i, std::forward<Args>(args)...);
  }
  return ptr;
}

template <typename PointerType, typename... Args>
constexpr auto construct_n(PointerType ptr, std::size_t n, Args &&...args) {
  for (std::size_t i = 0; i < n; ++i) {
    construct_at(ptr + i, std::forward<Args>(args)...);
  }
  return ptr;
}

// Version for synthetic pointer (has .get())
template <typename SrcPointerType, typename DestPointerType>
  requires requires { typename SrcPointerType::value_type; }
constexpr auto copy_construct_n(SrcPointerType src, std::size_t n,
                                DestPointerType dest) {
  for (std::size_t i = 0; i < n; ++i)
    construct_at(dest + i, *(src + i).get());
  return dest;
}

// Version for raw pointer
template <typename SrcPointerType, typename DestPointerType>
  requires(!requires { typename SrcPointerType::value_type; })
constexpr auto copy_construct_n(SrcPointerType src, std::size_t n,
                                DestPointerType dest) {
  for (std::size_t i = 0; i < n; ++i)
    construct_at(dest + i, *(src + i));
  return dest;
}

#if 0
template <typename SrcPointerType, typename DestPointerType>
constexpr auto copy_construct_n(SrcPointerType src, std::size_t n,
                                DestPointerType dest) {
  for (std::size_t i = 0; i < n; ++i) {
    construct_at(dest + i, *(src + i).get());
  }
  return dest;
}
#endif

template <typename T, typename MultitypeMemory>
constexpr void destroy_and_deallocate_n(MultitypeMemory &alloc, T *ptr,
                                        std::size_t n) {
  for (std::size_t i = 0; i < n; ++i) {
    std::destroy_at(ptr + i);
  }
  deallocate<T>(alloc, ptr, n);
}

template <typename T, typename MultitypeHandler>
constexpr auto get(MultitypeHandler &multitype_handler) {
  return multitype_handler.template get_handler<T>();
}

template <typename T, typename MultitypeHandler>
constexpr auto get(const MultitypeHandler &multitype_handler) -> const
    decltype(multitype_handler.template get_handler<T>()) {
  return multitype_handler.template get_handler<T>();
}

template <typename... RawType>
using dynamic_memory = multitype_memory<typed_dynamic_memory, RawType...>;

template <typename... RawType>
using default_memory = dynamic_memory<RawType...>;

template <typename... RawType>
using static_memory = multitype_memory<typed_static_memory, RawType...>;

template <typename T_Rebindable>
using static_t = rebind_memory_t<T_Rebindable,
                                 static_memory<relevant_types_t<T_Rebindable>>>;

} // namespace ycetl
