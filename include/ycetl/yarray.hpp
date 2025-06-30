#pragma once

template <typename Allocator, typename T, std::size_t N = 0> class array {
public:
};

template <typename Allocator, typename T> class darray {
  T *_data = nullptr;
  std::size_t _size = 0;

public:
  /*
  constexpr array(Allocator &allocator) : size(N) {
    static_assert(N > 0, "Size must be specified for array with N=0");
    data = allocate<Allocator, T>(N, allocator);
  }
  */
  constexpr darray(Allocator &allocator, std::size_t size)
      : _data(static_cast<T *>(
            static_cast<void *>(allocator.allocate(sizeof(T) * size)))),
        _size(size) {
    // data = static_cast<T *>(allocator.allocate(sizeof(T) *
    // initial_capacity));
    /*
    _data = static_cast<T *>(
        static_cast<void *>(allocator.allocate(sizeof(T) * initial_capacity)));
        */
  }
  constexpr void set_value(std::size_t index, const T &value) {
    if (_data == nullptr) {
      throw "Array is not initialized";
    }
    if (index >= _size) {
      throw "Index out of range";
    }
    _data[index] = value;
  }
  constexpr T &operator[](std::size_t index) {
    if (_data == nullptr) {
      throw "Array is not initialized";
    }
    if (index >= _size) {
      throw "Index out of range";
    }
    return _data[index];
  }
  constexpr const T &operator[](std::size_t index) const {
    if (_data == nullptr) {
      throw "Array is not initialized";
    }
    if (index >= _size) {
      throw "Index out of range";
    }
    return _data[index];
  }
};
