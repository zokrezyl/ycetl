#pragma once

#include <cstddef>
#include <ycetl/allocator_defs.hpp>
#include <ytrace/ytrace.hpp>

// clang-format off
namespace ycetl {
namespace memory {


// for allocator rebind this is the only object we ask
// the parront allocator to allocate
// and we guarantee that we will deallocate it
//
class _dynamic_storage_layout {
  void **_allocation_vector = nullptr;
  void **_array_allocation_vector = nullptr;

  const std::size_t _initial_vector_size = 0;
  const std::size_t _vector_growth_factor = 2;

  std::size_t _current_vector_size = 0;
  std::size_t _slots_taken = 0;

  std::size_t _current_array_vector_size = 0;
  std::size_t _array_slots_taken = 0;
};


// this is a dummy dynamic heap allocator
// that just stores the alloccated pointers
template <typename T> class dynamic_storage_impl {
  // this is the interface for the basic_storage that implements the aligned
  // allocation
  //
  // layout is allocated by the "parent" allocator who makes us rebind
  _dynamic_storage_impl_layout *_layout; 

public:
  constexpr _dynamic_storage_impl(): _layout(nullptr) {}

  constexpr ~dynamic_storage_impl() {
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

};

class type_wrapper {
    void* wrapped = nullptr;
    void (*deleter)(void*) = nullptr;

public:
    template <typename U>
    void from_foreign(U* u_ptr) {
        wrapped = u_ptr;
        deleter = [](void* p) { delete static_cast<U*>(p); };
    }

    ~Wrapper() {
        if (wrapped) deleter(wrapped);
    }
};

template <typename T> {
class dynamic_storage {
  _dynamic_storage_impl<T> *_impl;
  _dynamic_storage_impl<_dynamic_storage_impl<type_wrapper>> *_children = nullptr;
  bool _owns_impl = true;

public:
  T* allocato() {
    return impl->allocate();
  }
  T* allocate(std::size_t size) {
    return impl->allocate(size);
  }

  _dynamic_storage_impl<type_wrapper> *allocate_storage_wrapper(){
     _children->allocate(); 
  }

  dynamic_storage() : impl(new _dynamic_storage_impl<T>()) {

  }

  template <typename U> dynamic_storage(dynamic_storage<U> &other) {

  }

  template <typename U> struct rebind {
    using other = dynamic_storage<U>;
  };
}

  // clang-format on
} // namespace memory
} // namespace ycetl
