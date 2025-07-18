#pragma once

#include <cstddef>
#include <memory>

namespace ycetl {

// for allocator rebind this is the only object we ask
// the parront allocator to allocate
// and we guarantee that we will deallocate it
//

template <typename StoredType> struct size_and_pointer {
  StoredType *ptr = nullptr;
  std::size_t size = 0;
};
// this is a dummy dynamic heap allocator
// that just stores the alloccated pointers
template <typename StoredType> class typed_dynamic_memory {
public:
  using stored_type = StoredType;

private:
  size_and_pointer<stored_type> *_array_allocation_vector = nullptr;
  std::allocator<stored_type> alloc;

  const std::size_t _initial_vector_size = 100;
  const std::size_t _vector_growth_factor = 2;

  std::size_t _current_vector_size = 0;
  std::size_t _slots_taken = 0;

  std::size_t _current_array_vector_size = 0;
  std::size_t _array_slots_taken = 0;

public:
  constexpr typed_dynamic_memory() {}
  constexpr ~typed_dynamic_memory() {
    // same for the array allocation vector
    if (_array_allocation_vector != nullptr) {
      for (std::size_t i = 0; i < _array_slots_taken; ++i) {
        alloc.deallocate(
            _array_allocation_vector[i].ptr,
            _array_allocation_vector[i].size); // Use delete for single objects
      }
      delete[] _array_allocation_vector;
    }
  }

  constexpr stored_type *allocate(std::size_t size) {
    if (size == 0) {
      return nullptr;
    }
    // same as for single object allocation
    if (_current_array_vector_size == 0) {
      _array_allocation_vector =
          new size_and_pointer<stored_type>[_initial_vector_size];
      _current_array_vector_size = _initial_vector_size;
      _array_slots_taken = 0;
    } else {
      if (_array_slots_taken >= _current_array_vector_size) {
        // Resize the array allocation vector
        std::size_t new_size =
            _current_array_vector_size * _vector_growth_factor;
        size_and_pointer<stored_type> *new_vector =
            new size_and_pointer<stored_type>[new_size];
        for (std::size_t i = 0; i < _current_array_vector_size; ++i) {
          new_vector[i] = _array_allocation_vector[i];
        }
        delete[] _array_allocation_vector;
        _array_allocation_vector = new_vector;
        _current_array_vector_size = new_size;
      }
    }
    // stored_type *new_array = new stored_type[size]; // allocate an array of
    // stored_type
    stored_type *new_array = alloc.allocate(size);
    _array_allocation_vector[_array_slots_taken++] =
        size_and_pointer<stored_type>{
            new_array, size}; // store the pointer and size in the vector
    return new_array;
  }

  constexpr void deallocate(stored_type *ptr) {}

  constexpr void deallocate(stored_type *ptr, std::size_t size) {}
};

} // namespace ycetl
