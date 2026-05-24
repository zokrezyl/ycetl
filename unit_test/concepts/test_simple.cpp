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
  // Returns an offset, NOT a pointer. A constexpr value whose member
  // points into another of its own members is a subobject self-reference
  // and cannot be a constant expression — offsets sidestep that.
  constexpr std::size_t allocate(std::size_t n) {
    auto base = _offset;
    _offset += n;
    return base;
  }
  constexpr T &at(std::size_t i) { return _data[i]; }
  constexpr const T &at(std::size_t i) const { return _data[i]; }
};

template <typename T> struct static_container {
  static_memory_mock<T> _memory;
  std::size_t _size = 0;
  std::size_t _capacity = 0;
  std::size_t _offset = 0;
  bool _allocated = false;

public:
  constexpr static_container() = default;

  constexpr void push_back(const T &value) {
    if (!_allocated) {
      _offset = _memory.allocate(100);
      _capacity = 100;
      _allocated = true;
    }
    _memory.at(_offset + _size++) = value;
  }
  constexpr const T &operator[](std::size_t index) const {
    return _memory.at(_offset + index);
  }
  constexpr std::size_t size() const { return _size; }
};

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

  // static_container now stores an offset instead of a pointer into its
  // own _memory member, so it survives as a constexpr value.
  constexpr auto sc = get_static_container();
  static_assert(sc.size() == 2);
  static_assert(sc[0] == 1);
  static_assert(sc[1] == 2);
}
