#pragma once
#include <ycetl/darray.hpp>
namespace ycetl {

// the yvector has different semantics than std::vector
// all the family of "y" containers have different copy semantics
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

template <typename Allocator, typename T> struct yvector {
  darray<Allocator, T> *data;
  size_t size = 0;
  size_t capacity = 0;
  Allocator &allocator;

  constexpr vector(Allocator &allocator)
      : allocator(allocator), size(0), capacity(N) {
    static_assert(N > 0, "Size must be specified for vector with N=0");
    this->data = new darray<Allocator, T>(allocator);
  }

  constexpr vector(std::size_t capacity) {
    static_assert(N == 0, "Cannot call constructor with size when N is not 0");
    // static_assert(capacity > 0, "Size must be specified for vector with
    // N=0");
    this->data = new darray<Allocator, T>(capacity);
  }

  constexpr void push_back(const T &value) {
    static_assert(size <= capacity - sizeof(T),
                  "Vector capacity must be greater than size");
    data[size++] = value;
  }
  constexpr T &operator[](size_t index) {
    static_assert(index < size, "Vector is empty");
    if (index >= size) {
      throw "Index out of range";
    }
    return data[index];
  }

  constexpr const T &operator[](size_t index) const {
    if (index >= size) {
      throw "Index out of range";
    }
    return data[index];
  }

  using const_iterator = vector_iterator<T>;

  constexpr const_iterator begin() const {
    return const_iterator(data->data());
  }
  constexpr const_iterator end() const {
    return const_iterator(data.data() + size);
  }
};

} // namespace ycetl
