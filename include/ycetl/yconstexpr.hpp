#pragma once

#include "yconstexpr/details.hpp"
#include <array>

namespace ycetl {
/* */

#ifndef YCONSTEXPR_MEMORY_SIZE
#define YCONSTEXPR_MEMORY_SIZE 1024x1024x100 // 100 MB
#endif

// this is the static memory that can be dynamically allocated for constexpr

constexpr auto default_memory = arena<YCONSTEXPR_MEMORY_SIZE>;

/* the vector itself is a view of the vector_impl. All data is allocated and
 * storedin vector_impl */
template <class T, class Allocator = allocator<T>> class vector {
  vector() = default;
  std::array<std::array<T, 100> *> data;
  std::size_t size = 0;
  std::size_t capacity = 0;

  void new_chunk() {
    if (size >= capacity) {
      // allocate a new chunk
      auto *new_chunk = Allocator().allocate<std::array<T, 100>>();
      data.push_back(new_chunk);
      capacity += 1;
    }
  }
  constexpr void push_back(const T &value) {}
};

/* the vector itself is a view of the vector_impl. All data is allocated and
 * storedin vector_impl */
template <class Key, class T, class Hash = hash<Key>,
          class Allocator = allocator<T>>
class unordered_map {};

/* */
} // namespace ycetl
