#pragma once

#include <ycetl/darray.hpp>

namespace ycetl {

// the vector has similar interface to std::vector, but it is a simplified
//
template <typename T> struct vector_iterator {
  const T *ptr;

  constexpr vector_iterator(const T *p) : ptr(p) {}

  constexpr const T &operator*() const { return *ptr; }
  constexpr const T *operator->() const { return ptr; }

  constexpr vector_iterator &operator++() {
    ++ptr;
    return *this;
  }

  constexpr vector_iterator operator++(int) {
    vector_iterator tmp = *this;
    ++(*this);
    return tmp;
  }

  constexpr bool operator==(const vector_iterator &other) const {
    return ptr == other.ptr;
  }

  constexpr bool operator!=(const vector_iterator &other) const {
    return ptr != other.ptr;
  }
};

template <typename T> class vector {
private:
  darray<T> *data;
  ycetl::size_t size = 0;
  Allocator allocator;

  constexpr ycetl::size_t capacity() const { return data->size(); }

  constexpr vector() : data(), size(0) {}

  constexpr vector(ycetl::size_t capacity) : data(capacity), size(0) {}

  constexpr vector(Allocator allocator) : data(allocator), size(0) {}

  constexpr vector(Allocator allocator, ycetl::size_t capacity)
      : data(allocator, capacity), size(0) {}

  constexpr void push_back(const T &value) { data[size++] = value; }
  constexpr T &operator[](size_t index) { return data[index]; }

  constexpr const T &operator[](size_t index) const { return data[index]; }

  using const_iterator = vector_iterator<T>;

#if 0
  constexpr const_iterator begin() const {
    return const_iterator(data->data());
  }
  constexpr const_iterator end() const {
    return const_iterator(data.data() + size);
  }
#endif
};

} // namespace ycetl
