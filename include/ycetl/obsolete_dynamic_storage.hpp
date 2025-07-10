#pragma once

#include <cstddef>
#include <ycetl/storage_base.hpp>

namespace ycetl {
namespace storage {

// this is a dummy dynamic heap allocator
// that just stores the alloccated pointers
template <typename T> class dynamic_storage {
  T **_allocation_vector = nullptr;
  T **_array_allocation_vector = nullptr;

  const std::size_t _initial_vector_size = 0;
  const std::size_t _vector_growth_factor = 2;

  std::size_t _current_vector_size = 0;
  std::size_t _slots_taken = 0;

  std::size_t _current_array_vector_size = 0;
  std::size_t _array_slots_taken = 0;

  // this is the interface for the basic_storage that implements the aligned
  // allocation
public:
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

  // interface for the Allocator
  void *aligned_alloc(std::size_t _alignof, std::size_t _sizeof) {
    std::size_t bytes_remaining = (capacity - offset) * sizeof(T);
    if (!std::align(_alignof, _sizeof, aligned, bytes_remaining))
      throw std::bad_alloc();
    void *end_to_align = static_cast<std::byte *>(aligned) + _sizeof;
    if (!std::align(alignof(T), 1, end_to_align, bytes_remaining)) {
      throw std::bad_alloc(); // should never happen
    }
    offset = static_cast<T *>(end_to_align) - storage;
    impl->set_offset(offset);

    T *new_object = new T();
    // store the pointer in the allocation vector
    _allocation_vector[_slots_taken++] = new_object;
    return new_object;
  }

  // interface for the Allocator
  void *aligned_alloc(std::size_t _alignof, std::size_t _sizeof,
                      std::size_t num_elements) {
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

  void *_aligned_alloc(std::size_t _alignof, std::size_t _sizeof) {
    yinfo("Allocating aligned memory: alignof={}, sizeof={}", _alignof,
          _sizeof);
    // we use the default new operator for aligned allocation
    return this->aligned_alloc(_alignof, _sizeof);
  }

  void *_array_aligned_alloc(std::size_t _alignof, std::size_t _sizeof,
                             std::size_t num_elements) {
    yinfo("Allocating aligned array memory: alignof={}, sizeof={}, "
          "num_elements={}",
          _alignof, _sizeof, num_elements);
    // we use the default new operator for aligned allocation
    return this->aligned_array_alloc(_alignof, _sizeof, num_elements);
  }

  void _aligned_free(void *ptr) {
    yinfo("Freeing aligned memory: ptr={}", ptr);
    // we use the default delete operator for aligned deallocation
    // this->aligned_free(ptr);
  }

  void _array_aligned_free(void *ptr, std::size_t size) {
    yinfo("Freeing aligned array memory: ptr={}, size={}", ptr, size);
    // we use the default delete operator for aligned deallocation
    // this->aligned_array_free(ptr, size);
  }

  // we expose the vtable for static storage (for type erasure)
  static constexpr storage_vtable vtable{
      [](void *self, std::size_t _alignof, std::size_t _sizeof) {
        return static_cast<dynamic_storage *>(self)->_aligned_alloc(_alignof,
                                                                    _sizeof);
      },
      [](void *self, std::size_t _alignof, std::size_t _sizeof,
         std::size_t num_elements) {
        return static_cast<dynamic_storage *>(self)->_array_aligned_alloc(
            _alignof, _sizeof, num_elements);
      },
      [](void *self, void *ptr) {
        static_cast<dynamic_storage *>(self)->_aligned_free(ptr);
      },
      [](void *self, void *ptr, std::size_t sz) {
        static_cast<dynamic_storage *>(self)->_array_aligned_free(ptr, 0);
      },
  };
};

} // namespace storage
} // namespace ycetl
