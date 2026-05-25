// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <ycetl/allocator_defs.hpp>
#include <ytrace/ytrace.hpp>

// clang-format off
namespace ycetl {
namespace memory {

class type_wrapper {
    void* wrapped = nullptr;
    void (*deleter)(void*) = nullptr;

public:
    template <typename U>
 constexpr    void from_foreign(U* u_ptr) {
        wrapped = u_ptr;
        deleter = [](void* p) { delete static_cast<U*>(p); };
    }
    template <typename U>
 constexpr    type_wrapper(U* u_ptr) {
        wrapped = u_ptr;
        deleter = [](void* p) { delete static_cast<U*>(p); };
    }

    ~type_wrapper() {
        if (wrapped) deleter(wrapped);
    }
};

class type_array_wrapper {
    void* wrapped = nullptr;
    void (*deleter)(void*) = nullptr;

public:
    template <typename U>
 constexpr    void from_foreign(U* u_ptr) {
        wrapped = u_ptr;
        deleter = [](void* p) { delete [] static_cast<U*>(p); };
    }
    template <typename U>
    constexpr type_array_wrapper(U* u_ptr) {
        wrapped = u_ptr;
        deleter = [](void* p) { delete [] static_cast<U*>(p); };
    }

 constexpr    ~type_array_wrapper() {
        if (wrapped) deleter(wrapped);
    }
};


// for allocator rebind this is the only object we ask
// the parront allocator to allocate
// and we guarantee that we will deallocate it
//
struct _dynamic_storage_impl_data {
  // the allocation vector is a vector of pointers to pools of allocated objects
  type_wrapper **_allocation_vector = nullptr;
  // the array allocation vector is a vector of pointers to pools of allocated arrays of objects
  type_array_wrapper **_array_allocation_vector = nullptr;

  const std::size_t _initial_vector_size = 100;
  const std::size_t _vector_growth_factor = 2;

  std::size_t _current_vector_size = 0;
  std::size_t _slots_taken = 0;

  std::size_t _current_array_vector_size = 0;
  std::size_t _array_slots_taken = 0;

};


// this is a dummy dynamic heap allocator
// that just stores the alloccated pointers
template <typename T> class _dynamic_storage_impl {
  // this is the interface for the basic_storage that implements the aligned
  // allocation
  //
  // layout is allocated by the "parent" allocator who makes us rebind
  _dynamic_storage_impl_data *_data; 

public:
  constexpr _dynamic_storage_impl(): _data(nullptr) {}

  constexpr ~_dynamic_storage_impl() {
    if(_data == nullptr) {
      return; // nothing to deallocate
    }
    if (_data->_allocation_vector != nullptr) {
      for (std::size_t i = 0; i < _data->_current_vector_size; ++i) {
        delete[] _data->_allocation_vector[i];
      }
      delete[] _data->_allocation_vector;
    }

    // same for the array allocation vector
    if (_data->_array_allocation_vector != nullptr) {
      for (std::size_t i = 0; i < _data->_current_array_vector_size; ++i) {
        delete _data->_array_allocation_vector[i];
      }
      delete _data->_array_allocation_vector;
    }
    if(_data != nullptr) {
      delete _data;
      _data = nullptr;
    }
  }

  // Allocate memory for a single object of type T
  constexpr T *allocate() {
    if (_data == nullptr) {
      _data = new _dynamic_storage_impl_data{};
    }
    if (_data->_current_vector_size == 0) {
      _data->_allocation_vector = new type_wrapper *[_data->_initial_vector_size];
      _data->_current_vector_size = _data->_initial_vector_size;
      _data->_slots_taken = 0;
    } else {
      if (_data->_slots_taken >= _data->_current_vector_size) {
        // Resize the allocation vector
        std::size_t new_size = _data->_current_vector_size * _data->_vector_growth_factor;
        type_wrapper **new_vector = new type_wrapper *[new_size];
        for (std::size_t i = 0; i < _data->_current_vector_size; ++i) {
          new_vector[i] = _data->_allocation_vector[i];
        }
        delete[] _data->_allocation_vector;
        _data->_allocation_vector = new_vector;
        _data->_current_vector_size = new_size;
      }
    }
    T *new_object = new T();
    // store the pointer in the allocation vector
    _data->_allocation_vector[_data->_slots_taken++] = new type_wrapper(new_object);
    return new_object;
  }

  // Allocate memory for an array of objects of type T
constexpr T *allocate(std::size_t size) {
    if (size == 0) {
      return nullptr;
    }
    if (_data == nullptr) {
      _data =  new _dynamic_storage_impl_data{};
    }
    // same as for single object allocation
    if (_data->_current_array_vector_size == 0) {
      _data->_array_allocation_vector = new type_array_wrapper *[_data->_initial_vector_size];
      _data->_current_array_vector_size = _data->_initial_vector_size;
      _data->_array_slots_taken = 0;
    } else {
      if (_data->_array_slots_taken >= _data->_current_array_vector_size) {
        // Resize the array allocation vector
        std::size_t new_size =
            _data->_current_array_vector_size * _data->_vector_growth_factor;
        type_array_wrapper **new_vector = new type_array_wrapper *[new_size];
        for (std::size_t i = 0; i < _data->_current_array_vector_size; ++i) {
          new_vector[i] = _data->_array_allocation_vector[i];
        }
        delete[] _data->_array_allocation_vector;
        _data->_array_allocation_vector = new_vector;
        _data->_current_array_vector_size = new_size;
      }
    }
    T *new_array = new T[size]; // allocate an array of T
    _data->_array_allocation_vector[_data->_array_slots_taken++] = new type_array_wrapper(new_array);
    return new_array;
  }

  constexpr void deallocate(T *ptr) {
  }

  constexpr void deallocate(T *ptr, std::size_t size) {
  }

};


template <typename T>
class dynamic_storage {
  // the dynamic storage is implementing the core behaviour of the stl like Allocators rebind pattern
  // https://en.cppreference.com/w/cpp/memory/allocator_traits.html
  _dynamic_storage_impl<T> *_impl;
  bool _owns_impl = true;

public:
 constexpr  T* allocate(std::size_t size) {
    if (_impl == nullptr) {
      if (_owns_impl) {
        _impl = new _dynamic_storage_impl<T>();
      } else {
        // we are not the owner, so we just return nullptr
        return nullptr;
      }
    }
    return _impl->allocate(size);
  }
  constexpr void deallocate(T *ptr, std::size_t size) {
    if (_impl == nullptr) {
      return; // nothing to deallocate
    }
    if (_owns_impl) {
      _impl->deallocate(ptr, size);
    } else {
      // we are not the owner, so we just ignore the deallocation
      return;
    }
  }

  constexpr void deallocate(T *ptr) {
    if (_impl == nullptr) {
      return; // nothing to deallocate
    }
    if (_owns_impl) {
      _impl->deallocate(ptr);
    } else {
      // we are not the owner, so we just ignore the deallocation
      return;
    }
  }

 constexpr  _dynamic_storage_impl<type_wrapper> *allocate_storage_wrapper(){
     _children->allocate(); 
  }

 constexpr  dynamic_storage() : _impl(nullptr), _owns_impl(true), _children(nullptr) {

  }

  // destructor
  constexpr ~dynamic_storage() {
    if (_owns_impl && _impl != nullptr) {
      delete _impl;
    }
    if (_children != nullptr) {
      delete _children;
    }
  }

  template <typename U> 
constexpr 
  dynamic_storage(dynamic_storage<U> &other) {

  }

  template <typename U> struct rebind {
    using other = dynamic_storage<U>;
  };
};

// clang-format on
} // namespace memory
} // namespace ycetl
