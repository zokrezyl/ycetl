#pragma once

#include <cstddef>
#include <memory>

#include <ycetl/trivial_shared_ptr.hpp>

namespace ycetl {

// for allocator rebind this is the only object we ask
// the parront allocator to allocate
// and we guarantee that we will deallocate it
//
template <typename TypedDynamicMemory> class dynamic_synthetic_pointer {
public:
  using typed_memory = TypedDynamicMemory;
  using typed_memory_reference = typed_memory &;
  using this_type = dynamic_synthetic_pointer<TypedDynamicMemory>;
  using const_this_type = const dynamic_synthetic_pointer<TypedDynamicMemory>;
  using value_type = typename TypedDynamicMemory::value_type;
  using pointer = value_type *;
  using const_pointer = const pointer;
  using reference = value_type &;
  using const_reference = const value_type &;

private:
  typed_memory _typed_memory;
  pointer _ptr;

public:
  constexpr dynamic_synthetic_pointer() : _typed_memory(), _ptr(nullptr) {}

  constexpr dynamic_synthetic_pointer(typed_memory &typed_memory, pointer ptr)
      : _typed_memory(typed_memory), _ptr(ptr) {}

  constexpr pointer get() const { return _ptr; }
  constexpr const_reference operator*() const { return *_ptr; }
  constexpr reference operator*() { return *_ptr; }

  constexpr pointer operator->() const noexcept {
    return _ptr; // raw pointer (e.g. T*)
  }

  typed_memory_reference memory() { return _typed_memory; }
#if 0
  template <typename U> constexpr bool operator==(U *other) const {
    return _ptr == other;
  }

  template <typename U> constexpr bool operator!=(U *other) const {
    return _ptr != other;
  }
#endif

  constexpr bool operator==(pointer other) { return _ptr == other; }
  constexpr bool operator!=(pointer other) { return _ptr != other; }

  constexpr bool operator==(const_pointer other) const { return _ptr == other; }
  constexpr bool operator!=(const_pointer other) const { return _ptr != other; }

  constexpr dynamic_synthetic_pointer &operator++() {
    ++_ptr;
    return *this;
  }
  // postfix increment
  constexpr pointer operator++(int) {
    auto temp = *this;
    ++(*this);
    return temp;
  }

  constexpr bool operator==(const dynamic_synthetic_pointer &other) const {
    return _ptr == other._ptr;
  }

  constexpr bool operator!=(const dynamic_synthetic_pointer &other) const {
    return _ptr != other._ptr;
  }

  constexpr reference operator[](std::size_t index) { return _ptr[index]; }
  constexpr const_reference operator[](std::size_t index) const {
    return _ptr[index];
  }

  constexpr this_type operator+(std::size_t offset) {
    return this_type{_typed_memory, _ptr + offset};
  }

  constexpr const_this_type operator+(std::size_t offset) const {
    return this_type{_typed_memory, _ptr + offset};
  }
};

template <typename T> class typed_dynamic_memory;

template <typename StoredType> struct size_and_pointer {
  StoredType *ptr = nullptr;
  std::size_t size = 0;
};
// this is a dummy dynamic heap allocator
// that just stores the alloccated pointers
template <typename StoredType> class typed_dynamic_memory_backend {
public:
  using stored_type = StoredType;
  using pointer = StoredType *;

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
  constexpr typed_dynamic_memory_backend() {}
  constexpr ~typed_dynamic_memory_backend() {
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

  constexpr pointer allocate(std::size_t size) {
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

  constexpr void deallocate(pointer ptr) {}

  constexpr void deallocate(pointer /*ptr*/, std::size_t /*size*/) {}
};

template <typename StoredType>
class typed_dynamic_memory
    : public trivial_shared_ptr<typed_dynamic_memory_backend<StoredType>> {
public:
  using value_type = StoredType;
  using this_type = typed_dynamic_memory<StoredType>;
  using backend_type = typed_dynamic_memory_backend<StoredType>;
  using base_type = trivial_shared_ptr<backend_type>;
  using stored_type = StoredType;
  using pointer = dynamic_synthetic_pointer<this_type>;
  using const_pointer = const pointer;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  constexpr typed_dynamic_memory() : base_type(new backend_type()) {}

  constexpr pointer allocate(std::size_t n) {
    return pointer(*this, this->get()->allocate(n));
  }

  constexpr void deallocate(pointer p, std::size_t n) {
    this->get()->deallocate(p.get(), n);
  }

  constexpr pointer none() { return pointer(*this, nullptr); }
};

} // namespace ycetl
