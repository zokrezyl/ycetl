#pragma once

#include <cstddef>
#include <ycetl/storage.hpp>
#include <ycetl/storage_base.hpp>

namespace ycetl {
namespace storage {

// this is a dummy dynamic heap allocator
// that just stores the alloccated pointers
template <typename T>
class dynamic_storage : public storage_base<T, dynamic_storage<T>> {
  T **_allocation_vector = nullptr;
  T **_array_allocation_vector = nullptr;

  const std::size_t _initial_vector_size = 0;
  const std::size_t _vector_growth_factor = 2;

  std::size_t _current_vector_size = 0;
  std::size_t _slots_taken = 0;

  std::size_t _current_array_vector_size = 0;
  std::size_t _array_slots_taken = 0;

public:
  using value_type = T;
  // lazy initialization of the data pointer
  constexpr dynamic_storage() {}

  constexpr ~dynamic_storage() {
    if (_allocation_vector != nullptr) {
      for (std::size_t i = 0; i < _current_vector_size; ++i) {
        delete[] _allocation_vector[i];
      }
      delete[] _allocation_vector;
    }

    // same for the array allocation vector
    if (_array_allocation_vector != nullptr) {
      for (std::size_t i = 0; i < _current_array_vector_size; ++i) {
        delete[] _array_allocation_vector[i];
      }
      delete[] _array_allocation_vector;
    }
  }

  // Allocate memory for a single object of type T
  T *allocate() {
    if (_current_vector_size == 0) {
      _allocation_vector = new T *[_initial_vector_size];
      _current_vector_size = _initial_vector_size;
      _slots_taken = 0;
    } else {
      if (_slots_taken >= _current_vector_size) {
        // Resize the allocation vector
        std::size_t new_size = _current_vector_size * _vector_growth_factor;
        T **new_vector = new T *[new_size];
        for (std::size_t i = 0; i < _current_vector_size; ++i) {
          new_vector[i] = _allocation_vector[i];
        }
        delete[] _allocation_vector;
        _allocation_vector = new_vector;
        _current_vector_size = new_size;
      }
    }
    T *new_object = new T();
    // store the pointer in the allocation vector
    _allocation_vector[_slots_taken++] = new_object;
    return new_object;
  }

  // Allocate memory for an array of objects of type T
  T *allocate(std::size_t size) {
    if (size == 0) {
      return nullptr;
    }
    // same as for single object allocation
    if (_current_array_vector_size == 0) {
      _array_allocation_vector = new T *[_initial_vector_size];
      _current_array_vector_size = _initial_vector_size;
      _array_slots_taken = 0;
    } else {
      if (_array_slots_taken >= _current_array_vector_size) {
        // Resize the array allocation vector
        std::size_t new_size =
            _current_array_vector_size * _vector_growth_factor;
        T **new_vector = new T *[new_size];
        for (std::size_t i = 0; i < _current_array_vector_size; ++i) {
          new_vector[i] = _array_allocation_vector[i];
        }
        delete[] _array_allocation_vector;
        _array_allocation_vector = new_vector;
        _current_array_vector_size = new_size;
      }
    }
    T *new_array = new T[size]; // allocate an array of T
    _array_allocation_vector[_array_slots_taken++] = new_array;
    return new_array;
  }

  // Deallocate memory
  constexpr void deallocate(T *ptr) {
    throw std::runtime_error(
        "Deallocation is not supported in dynamic_storage. "
        "Use the destructor to free memory.");
  }
  constexpr void deallocate(T *ptr, std::size_t) {
    throw std::runtime_error(
        "Deallocation is not supported in dynamic_storage. "
        "Use the destructor to free memory.");
  }

  // we expose the vtable for static storage (for type erasure)
  template <typename U>
  static constexpr storage_vtable<U> vtable{
      [](void *self) {
        return static_cast<dynamic_storage *>(self)->aligned_alloc();
      },
      [](void *self, std::size_t sz) {
        return static_cast<dynamic_storage *>(self)->allocate(sz);
      },
      [](void *self, U *ptr) {
        static_cast<dynamic_storage *>(self)->deallocate(ptr);
      },
      [](void *self, U *ptr, std::size_t sz) {
        static_cast<dynamic_storage *>(self)->deallocate(ptr, 0);
      },
  };
};

} // namespace storage
} // namespace ycetl
