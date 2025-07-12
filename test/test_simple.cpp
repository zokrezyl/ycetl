#include <ycetl/memory.hpp>
#include <ycetl/dynamic_array.hpp>

#include <iostream>
#include <type_traits>
#include <stdexcept>

#include <memory>

using namespace ycetl;

struct A{
  int i;
  int j;
  constexpr A(int i, int j) : i(1), j(1) {
  }
};

struct B{
  int i;
  int j;
  A a;
};

struct C{
  int i;
  int j;
  A a;
  B b;
};

constexpr int test0() {
  if (std::is_constant_evaluated())
  {
    return 1;
  }
  else { 
    return 0;
  }
}

/*
* vector<vector<int>> v;
* v[0] will return instead of 'vector<int> &' 'vector<int>'
* vector<A> a;
* a[0] will return instead of 'A &' 'A'
*/

template <typename T>
class container_impl {
public:
  T *_data;
  constexpr container_impl() : _data(new T())
  {
  }

  // copy constructor
 constexpr  container_impl(const container_impl &other) {
    //_data = new T(*other._data);
    _data = new T{};
  }
  // destructor
 constexpr  ~container_impl() {
    delete _data;
  }

  // copy assignment - MINIMUM IMPLEMENTATION NEEDED
  constexpr container_impl& operator=(const container_impl &other) {
    if (this != &other) {
      *_data = *other._data;  // Reuse existing allocation instead of delete+new
    }
    return *this;
  }
  

};

template <typename T>
class container {
public:
  container_impl<T> *impl;
 constexpr  container() : impl(new container_impl<T>) {
  }
  // copy constructor
 constexpr  container(const container &other) {
   impl = new container_impl<T>(*other.impl) ;
  }
  // destructor
 constexpr  ~container() {
    delete impl;
  }

  // copy assignment - MINIMUM IMPLEMENTATION NEEDED
  constexpr container& operator=(const container &other) {
    if (this != &other) {
      *impl = *other.impl;  // Use container_impl's assignment operator
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
  constexpr ~simple_inner() {
    delete ptr;
  }
  // copy constructor
  constexpr simple_inner(const simple_inner &other) {
      ptr = new int(*other.ptr);
  }

};


struct simple_container {
  simple_inner *ptr;
 constexpr  simple_container() : ptr(new simple_inner()) {}
 constexpr  ~simple_container() {
    delete ptr;
  }
  // copy constructor
  constexpr  simple_container(const simple_container &other) {
      ptr = new simple_inner(*other.ptr);
  }
};

simple_container constexpr create_sc() {
  simple_container sc{};
  *sc.ptr->ptr = 42;
  return sc;
}

constexpr auto test( ) {
  simple_container sc = create_sc();
  return *sc.ptr->ptr; // Should return 42
};


int main() {
  constexpr auto result = test();
  std::cout << "Result: " << result << std::endl;
}



