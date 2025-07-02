#pragma once
#include <ycetl/dynamic_arena.hpp>

namespace ycetl::object_pool {

template <typename T> class object_pool {
private:
  ycetl::arena::dynamic_arena<T> *_arena;
  ycetl::arena::dynamic_arena<T *> *_objects;

public:
  constexpr object_pool() : _arena(nullptr) {}

  // adds an object to the pool
  constexpr std::size_t add(T &t) {
    if (_arena == nullptr) {
      // lazy initialization of the arena
      _arena = new ycetl::arena::arena<T>();
    }
    return _arena->add(t);
  }

  // destructor
  ~object_pool() {
    if (_arena) {
      delete _arena;
    }
  }
};

} // namespace ycetl::object_pool
