#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>

#include <array>
#include <iostream>
#include <stdexcept>
#include <type_traits>

#include <memory>

using namespace ycetl;

constexpr int test0() {
  if (std::is_constant_evaluated()) {
    return 1;
  } else {
    return 0;
  }
}

template <typename T> class container_impl {
public:
  T *_data;
  constexpr container_impl() : _data(new T()) {}

  // copy constructor
  constexpr container_impl(const container_impl &other) {
    //_data = new T(*other._data);
    _data = new T{};
  }
  // destructor
  constexpr ~container_impl() { delete _data; }

  // copy assignment - MINIMUM IMPLEMENTATION NEEDED
  constexpr container_impl &operator=(const container_impl &other) {
    if (this != &other) {
      *_data = *other._data; // Reuse existing allocation instead of delete+new
    }
    return *this;
  }
};

template <typename T> class container {
public:
  container_impl<T> *impl;
  constexpr container() : impl(new container_impl<T>) {}
  // copy constructor
  constexpr container(const container &other) {
    impl = new container_impl<T>(*other.impl);
  }
  // destructor
  constexpr ~container() { delete impl; }

  // copy assignment - MINIMUM IMPLEMENTATION NEEDED
  constexpr container &operator=(const container &other) {
    if (this != &other) {
      *impl = *other.impl; // Use container_impl's assignment operator
    }
    return *this;
  }
};

constexpr container<int> make_container() {
  container<int> c{};
  *c.impl->_data = 10;
  return c;
}

struct simple_inner {
  int *ptr;
  constexpr simple_inner() : ptr(new int(0)) {}
  constexpr ~simple_inner() { delete ptr; }
  // copy constructor
  constexpr simple_inner(const simple_inner &other) {
    ptr = new int(*other.ptr);
  }
};

struct simple_container {
  simple_inner *ptr;
  constexpr simple_container() : ptr(new simple_inner()) {}
  constexpr ~simple_container() { delete ptr; }
  // copy constructor
  constexpr simple_container(const simple_container &other) {
    ptr = new simple_inner(*other.ptr);
  }
};

simple_container constexpr create_sc() {
  simple_container sc{};
  *sc.ptr->ptr = 42;
  return sc;
}

constexpr auto test() {
  simple_container sc = create_sc();
  return *sc.ptr->ptr; // Should return 42
};

constexpr auto test_container() {
  container<int> c;
  return c;
}

template <typename T> class simple_static_container {
  std::array<T, 1024> _data;
  std::size_t _capacity = 1024;
  std::size_t _size = 0;

public:
  constexpr simple_static_container() : _data{} {}
  constexpr void push_back(const T &value) { _data[_size++] = value; }
  constexpr T &operator[](std::size_t index) { return _data[index]; }
  constexpr const T &operator[](std::size_t index) const {
    return _data[index];
  }
  constexpr std::size_t size() const { return _size; }
};

template <typename T> class static_memory_mock {
  T _data[1024];
  std::size_t _capacity = 1024;
  std::size_t _offset = 0;

public:
  constexpr static_memory_mock() : _data{} {}
  constexpr T *allocate(std::size_t n) {
    T *ptr = &_data[_offset];
    _offset += n;
    return ptr;
  }
};
// clang-format off
template <typename T> struct static_container {
  static_memory_mock<T> _memory;
  std::size_t _size = 0;
  std::size_t _capacity = 0; // Default capacity
  T *_data = nullptr;

public:
  constexpr static_container() : _memory(), _data{} {}
  constexpr void push_back(int value) { 
    if (_data == nullptr) {
      _data = _memory.allocate(100);
      _capacity = 100;
    }
    _data[_size++] = value;
  }
  constexpr int operator[](std::size_t index) const { 
    return _data + index;
  }
  // copy constructor
  constexpr static_container(const static_container &other): _memory() {
    _data = _memory.allocate(other._capacity);
    _size = other._size;
    _capacity = other._capacity;
  }
  // copy operator
  constexpr static_container &operator=(const static_container &other) {
    if (this != &other) {
      _data = _memory.allocate(other._capacity);
      _size = other._size;
      _capacity = other._capacity;
      for (std::size_t i = 0; i < _size; ++i) {
        _data[i] = other._data[i];
      }
    }
    return *this;
  }
};

// clang-format on

constexpr auto get_simple_static_container() {
  simple_static_container<int> sc;
  sc.push_back(1);
  sc.push_back(2);
  return sc;
}

constexpr auto get_static_container() {
  static_container<int> sc;
  sc.push_back(1);
  sc.push_back(2);
  return sc;
}

int main() {
  constexpr auto result = test();
  std::cout << "Result: " << result << std::endl;
  constexpr auto m = get_simple_static_container();
  static_assert(m.size() == 2, "First element should be 1");
  static_assert(m[0] == 1, "First element should be 1");

  constexpr auto sc = get_static_container();
}
