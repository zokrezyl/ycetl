#pragma once

#include <cstddef>

#include <ycetl/dynamic_storage.hpp>
#include <ycetl/storage_base.hpp>

namespace ycetl {
namespace storage {

// function pointer based type erasurement for storage backends
// as polymorphism is not allowed in constexpr contexts
//
// The size parameter must be an integral multiple of alignment.
// as per https://en.cppreference.com/w/cpp/memory/c/aligned_alloc
//

// storage is a type-erased storage backend that can be used to allocate
// aligned memory in a constexpr context.
// It uses a vtable to call the appropriate functions for the backend.
// If no  backend is provided, it will use a dynamic_storage backend
class storage {
  void *_backend = nullptr;
  const storage_vtable *_vtable;
  bool owns_backend = false;

public:
  // constexpr storage() : _storage_impl(nullptr) {}
  //
  constexpr storage() : _backend{nullptr}, _vtable{nullptr} {}

  template <typename Backend>
  constexpr storage(Backend *b) : _backend{b}, _vtable{&Backend::vtable} {}

  constexpr void assure_backend() {
    if (_backend == nullptr || _vtable == nullptr) {
      _backend = static_cast<void *>(new dynamic_storage<char>());
      _vtable = &dynamic_storage<char>::vtable;
      owns_backend = true;
    }
  }

  constexpr void *aligned_alloc(std::size_t _align, std::size_t _size) {
    assure_backend();
    return _vtable->aligned_alloc(_backend, _align, _size);
  }

  constexpr void *array_aligned_alloc(std::size_t _align, std::size_t _size,
                                      std::size_t num_elements) {
    assure_backend();
    return _vtable->array_aligned_alloc(_backend, _align, _size, num_elements);
  }

  constexpr void aligned_free(void *ptr) {
    assure_backend();
    _vtable->aligned_free(_backend, ptr);
  }

  constexpr void array_aligned_free(void *ptr, std::size_t size) {
    assure_backend();
    _vtable->array_aligned_free(_backend, ptr, size);
  }

  constexpr ~storage() {
    if (owns_backend && _backend != nullptr) {
      delete static_cast<dynamic_storage<char> *>(_backend);
    }
  }
};

} // namespace storage
} // namespace ycetl
