// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <ytrace/ytrace.hpp>

namespace ycetl {
namespace storage {

struct storage_vtable {
  void *(*aligned_alloc)(
      void * /*this*/, std::size_t /*alignof*/,
      std::size_t /*sizeof*/); // second parameter is used for
                               // typed allocate with aligmnment
  void *(*array_aligned_alloc)(
      void * /*this*/, std::size_t /*alignof*/, std::size_t /*sizeof*/,
      std::size_t /*num of elements in array*/); // last parameter is for the
                                                 // typed array allocation with
                                                 // alignment
  void (*aligned_free)(void *, void *);          // second parameter is used for
                                        // typed deallocate with alignment
  void (*array_aligned_free)(void *, void *,
                             std::size_t); // last parameter is for the typed
                                           // array deallocation with alignment
};

template <typename T, typename StorageImpl> class storage_base {
public:
  // for array allocation
  // align: alignment in bytes of the target
  // size: size of the target in bytes
  // num: number of elements to allocate
  void *array_aligned_alloc(std::size_t _alignof, std::size_t _sizeof,
                            std::size_t num) {
    return nullptr;
  }

  // for array allocation
  // align: alignment in bytes of the target
  // size: size of the target in bytes
  // num: number of elements to allocate
  // the memory is not initialized, thus the caller is responsible for
  // initializing the memory by calling new(buffer) U (or so)
  void *aligned_alloc(std::size_t _alignof, std::size_t _sizeof) {
    yinfo("Allocating aligned memory: alignof={}, sizeof={}", _alignof,
          _sizeof);

    StorageImpl *impl = static_cast<StorageImpl *>(this);

    auto offset = impl->offset();
    auto capacity = impl->capacity();
    auto storage = impl->storage();

    yinfo("Storage capacity: {}, offset: {}, storage: {}", capacity, offset,
          (void *)storage);
    T *current = storage + offset;

    std::size_t bytes_remaining = (capacity - offset) * sizeof(T);

    void *aligned = static_cast<void *>(current);

    yinfo("Current pointer: {}, bytes_remaining: {}", (void *)aligned,
          bytes_remaining);
    if (!std::align(_alignof, _sizeof, aligned, bytes_remaining))
      throw std::bad_alloc();

    yinfo("Aligned pointer: {}, bytes_remaining: {}", (void *)aligned,
          bytes_remaining);

    // Move past U in bytes
    void *end_to_align = static_cast<std::byte *>(aligned) + _sizeof;
    // Align end to T using remaining space after U
    if (!std::align(alignof(T), 1, end_to_align, bytes_remaining)) {
      throw std::bad_alloc(); // should never happen
    }

    // bytes_remaining is as if we would have allocated next T after U
    offset = static_cast<T *>(end_to_align) - storage;
    impl->set_offset(offset);
    return aligned;
  }
};

} // namespace storage
} // namespace ycetl
