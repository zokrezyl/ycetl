#pragma once

#include <utility>
#include <ytrace/ytrace.hpp>

namespace ycetl {
namespace storage {

// provides a convenience interfae to create an object of type T using a storage
// backend the storage must implement the following methods:
//
template <typename Storage> class factory {
  Storage *storage;

public:
  factory(Storage *storage) : storage(storage) {
    yinfo("factory created with storage: {}", (void *)storage);
  }

  // returns an uninitialized memory block of size sizeof(T)
  template <typename T> void *get_memory() {
    yinfo("Allocating memory for type: {}", typeid(T).name());
    return storage->aligned_alloc(
        alignof(T), sizeof(T)); // allocate memory with alignment for type T
  }

  // create an object of type T
  template <typename T, typename... Args> T *create(Args &&...args) {
    yinfo("Creating object of type: {}", typeid(T).name());
    void *memory = get_memory<T>();
    return new (memory) T(std::forward<Args>(args)...);
  }

  // destroy an object of type T
  template <typename T>
  void destroy(T *object) { /*storage->destroy(object); */ }
};

} // namespace storage
} // namespace ycetl
