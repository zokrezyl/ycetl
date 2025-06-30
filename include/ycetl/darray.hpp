#pragma once
#include <ycetl/memory.hpp>

namespace ycetl {

// the darray has the interface of std::array, but can use the allocator behind
// the darray my inherit the allocator from the allocator of the darray
// or allocate itself the memory
template <typename T> class darray {
  ycetl::Allocator _allocator;
  ycetl::size_t _size;
  T *_data;

public:
  constexpr darray() : _allocator(), _size(0), _data(nullptr) {}

  // lazy initialization
  constexpr darray(ycetl::size_t _size)
      : _allocator(), _size(_size), _data(nullptr) {}

  constexpr darray(ycetl::Allocator allocator)
      : _allocator(allocator), _size(0), _data(nullptr) {}

  constexpr darray(ycetl::Allocator allocator, ycetl::size_t _size)
      : _allocator(allocator), _size(_size), _data(nullptr) {}

  constexpr Allocator allocator() const { return _allocator; }
  // Returns the number of elements in the container.
  constexpr ycetl::size_t size() const { return _size; }
  // destructor
  constexpr ~darray() {}
};

} // namespace ycetl
