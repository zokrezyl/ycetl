#pragma once

#include <cstddef>

namespace ycetl {
namespace storage {

// function pointer based type erasurement for storage backends
// as polymorphism is not allowed in constexpr contexts
//
// The size parameter must be an integral multiple of alignment.
// as per https://en.cppreference.com/w/cpp/memory/c/aligned_alloc
//
template <typename T> struct storage_vtable {
  T *(*aligned_alloc)(void * /*this*/, std::size_t /*alignof*/,
                      std::size_t /*sizeof*/); // second parameter is used for
                                               // typed allocate with aligmnment
  T *(*array_aligned_alloc)(
      void * /*this*/, std::size_t /*alignof*/, std::size_t /*sizeof*/,
      std::size_t /*num of elements in array*/); // last parameter is for the
                                                 // typed array allocation with
                                                 // alignment
  void (*aligned_free)(void *, T *,
                       std::size_t); // second parameter is used for typed
                                     // deallocate with alignment
  void (*vector_aligned_free)(void *, T *, std::size_t,
                              std::size_t); // last parameter is for the typed
                                            // array deallocation with alignment
};

template <typename T> class storage {
  void *_backend;
  const storage_vtable<T> *_vtable;

public:
  // constexpr storage() : _storage_impl(nullptr) {}

  template <typename Backend>
  constexpr storage(Backend *b)
      : _backend{b}, _vtable{&Backend::template vtable<T>} {}

  constexpr T *aligned_alloc(std::size_t _align, std::size_t _size) {
    return _vtable->aligned_alloc(_backend, _align, _size);
  }

  constexpr T *array_aligned_alloc(std::size_t _align, std::size_t _size,
                                   std::size_t num_elements) {
    return _vtable->aligned_alloc(_backend, _align, _size, num_elements);
  }

  constexpr void deallocate(T *ptr) {
    _vtable->deallocate_single(_backend, ptr, alignof(T));
  }

  constexpr void deallocate(T *ptr, std::size_t size) {
    _vtable->deallocate_array(_backend, ptr, size, alignof(T));
  }

  template <typename U> constexpr void deallocate(T *ptr) {
    _vtable->deallocate_single(_backend, ptr, alignof(U));
  }

  template <typename U> constexpr void deallocate(T *ptr, std::size_t size) {
    _vtable->deallocate_array(_backend, ptr, size, alignof(U));
  }
};

template <typename Backend>
storage(Backend *) -> storage<typename Backend::value_type>;

} // namespace storage
} // namespace ycetl
