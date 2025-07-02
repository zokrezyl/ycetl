#pragma once

#include <memory>

#include <ytrace/ytrace.hpp>

namespace ycetl {
namespace storage {

#include <ycetl/storage_base.hpp>

// this is a dummy dynamic heap allocator
// that just stores the alloccated pointers
template <typename T>
class static_storage : public storage_base<T, static_storage<T>> {

  T *_storage = nullptr;
  std::size_t _capacity = 0; // strictly in sizof(T)
  std::size_t _offset = 0;

  void assure_storage() {
    // for the moment  we assume that the storage is always initialized
    if (_storage == nullptr) {
      throw std::runtime_error("Storage is not initialized.");
    }
  }

  // this is the interface for the basic_storage that implements the aligned
  // allocation
public:
  T *storage() { return _storage; }

  std::size_t capacity() const { return _capacity; }

  std::size_t offset() const { return _offset; }

  void set_offset(std::size_t offset) {
    if (offset > _capacity) {
      throw std::out_of_range("Offset exceeds storage capacity.");
    }
    _offset = offset;
  }

public:
  using value_type = T;
  constexpr static_storage() {}

  // Allocate memory for a single object of type T
  constexpr T *allocate() {
    assure_storage();
    T *ptr = _storage + _offset;
    _offset += sizeof(T);
    return ptr;
  }

  constexpr T *allocate(std::size_t size) {
    assure_storage();
    if (size == 0) {
      return nullptr;
    }
    T *ptr = _storage + _offset;
    _offset += size * sizeof(T);
    return ptr;
  }

  template <typename U> constexpr U *__allocate() {
    assure_storage();
    // we need allignment logic
    T *current = _storage + _offset;
    std::size_t bytes_remaining = (_capacity - _offset) * sizeof(T);
    void *aligned_to_U = static_cast<void *>(current);

    if (!std::align(alignof(U), sizeof(U), aligned_to_U, bytes_remaining))
      throw std::bad_alloc();

    U *aligned_new = new (aligned_to_U) U;
    // Move past U in bytes
    void *end_to_align = static_cast<std::byte *>(aligned_to_U) + sizeof(U);
    // Align end to T using remaining space after U
    if (!std::align(alignof(T), 1, end_to_align, bytes_remaining)) {
      throw std::bad_alloc(); // should never happen
    }
    // bytes_remaining is as if we would have allocated next T after U
    _offset = static_cast<T *>(end_to_align) - _storage;

    return aligned_new;
  }

  template <typename U> constexpr U *__allocate(std::size_t size) {
    assure_storage();
    if (size == 0) {
      return nullptr;
    }
    U *ptr = _storage + _offset;
    _offset += size * sizeof(U);
    return ptr;
  }

  constexpr static_storage(T *storage, std::size_t _size)
      : _storage(storage), _capacity(_size) {}

  template <std::size_t N>
  constexpr static_storage(std::array<T, N> &_storage)
      : _storage(_storage.data()), _capacity(_storage.size()) {}

  void *_aligned_alloc(std::size_t _alignof, std::size_t _sizeof) {
    yinfo("Allocating aligned memory: alignof={}, sizeof={}", _alignof,
          _sizeof);
    return this->aligned_alloc(_alignof, _sizeof);
  }

  void *_array_aligned_alloc(std::size_t _alignof, std::size_t _sizeof,
                             std::size_t num_elements) {
    yinfo("Allocating aligned array memory: alignof={}, sizeof={}, "
          "num_elements={}",
          _alignof, _sizeof, num_elements);
    return this->array_aligned_alloc(_alignof, _sizeof, num_elements);
  }

  template <typename U>
  static constexpr storage_vtable<U> vtable{
      [](void *self, std::size_t _alignof, std::size_t _sizeof) {
        return static_cast<static_storage *>(self)->_aligned_alloc(_alignof,
                                                                   _sizeof);
      },
      [](void *self, std::size_t _alignof, std::size_t _sizeof,
         std::size_t num_elements) {
        return static_cast<static_storage *>(self)->_array_aligned_alloc(
            _alignof, _sizeof, num_elements);
      },
      [](void *self, U *ptr) {
        static_cast<static_storage *>(self)->deallocate(ptr);
      },
      [](void *self, U *ptr, std::size_t sz) {
        static_cast<static_storage *>(self)->deallocate(ptr, 0);
      },
  };
};

} // namespace storage
} // namespace ycetl
