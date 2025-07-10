#pragma once

#include <cstddef>
#include <ycetl/allocator_defs.hpp>
// #include <ytrace/ytrace.hpp>

namespace ycetl {
namespace memory {

// for allocator rebind this is the only object we ask
// the parront allocator to allocate
// and we guarantee that we will deallocate it
//

// this is a dummy dynamic heap allocator
// that just stores the alloccated pointers
template <typename T> class dynamic_storage {

  T **_allocation_vector = nullptr;
  T **_array_allocation_vector = nullptr;

  const std::size_t _initial_vector_size = 100;
  const std::size_t _vector_growth_factor = 2;

  std::size_t _current_vector_size = 0;
  std::size_t _slots_taken = 0;

  std::size_t _current_array_vector_size = 0;
  std::size_t _array_slots_taken = 0;
  // this is the interface for the basic_storage that implements the aligned
  // allocation
  //

public:
  constexpr dynamic_storage() {}
  constexpr ~dynamic_storage() {
    if (_allocation_vector != nullptr) {
      for (std::size_t i = 0; i < _slots_taken; ++i) {
        delete _allocation_vector[i]; // Use delete for single objects
      }
      delete[] _allocation_vector;
    }

    // same for the array allocation vector
    if (_array_allocation_vector != nullptr) {
      for (std::size_t i = 0; i < _array_slots_taken; ++i) {
        delete[] _array_allocation_vector[i]; // Use delete[] for arrays
      }
      delete[] _array_allocation_vector;
    }
  }

  // Allocate memory for a single object of type T
  constexpr T *allocate() {
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
  constexpr T *allocate(std::size_t size) {
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

  constexpr void deallocate(T *ptr) {}

  constexpr void deallocate(T *ptr, std::size_t size) {}

  template <typename U> constexpr dynamic_storage(dynamic_storage<U> &other) {}

  template <typename U> struct rebind {
    using other = dynamic_storage<U>;
  };
};

} // namespace memory
} // namespace ycetl
