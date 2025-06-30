#pragma once
#include <ycetl/allocator.hpp>

// helper class to use cetl infrastructure
//
template <typename T, std::size_t arena_size = 10_MB> class stl_user {
public:
  // arena<arena_size> a;
  RomAllocator<arena_size> allocator;
  // constexpr stl_user() { _singleton = *this; }

public:
  constexpr stl_user() : allocator() {
    // This constructor initializes the arena and allocator
  }
  /*
  using _vector = vector<_stl_user, T>;
  using _string = string<_stl_user>>;
  */
};
