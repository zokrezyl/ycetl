// SPDX-License-Identifier: MIT

#pragma once
#include <array>
#include <cstddef>
#include <stdexcept>

namespace ycetl {

constexpr std::size_t typed_static_memory_size = 1024;

template <typename TypedStaticMemory> struct static_synthetic_pointer {
public:
  using typed_memory = TypedStaticMemory;
  using typed_memory_reference = typed_memory &;
  using size_type = typename typed_memory::size_type;

  using raw_value = typename typed_memory::raw_value;
  using raw_pointer = typename typed_memory::raw_pointer;
  using const_raw_pointer = typename typed_memory::const_raw_pointer;
  using raw_reference = typename typed_memory::raw_reference;
  using const_raw_reference = typename typed_memory::const_raw_reference;

  using difference_type = typename typed_memory::difference_type;
  using pointer = static_synthetic_pointer<TypedStaticMemory>;
  using const_pointer = const pointer;

private:
  // Pointer (not reference) so the class stays copy-/move-assignable, which
  // is required by dynamic_array's reserve()/clear()/operator=.
  typed_memory *_typed_memory = nullptr;
  std::size_t _offset = 0; // offset 0 has nullptr semantics
public:
  constexpr static_synthetic_pointer() noexcept = default;

  constexpr static_synthetic_pointer(typed_memory &mem, size_type offset)
      : _typed_memory(&mem), _offset(offset) {}

  constexpr void reset() noexcept { _offset = 0; }

  constexpr raw_reference operator[](size_type index) {
    return (*_typed_memory)[_offset + index];
  }
  constexpr const_raw_reference operator[](std::size_t index) const {
    return (*_typed_memory)[_offset + index];
  }
  constexpr raw_pointer get() {
    return _typed_memory ? &(*_typed_memory)[_offset] : nullptr;
  }
  constexpr const_raw_pointer get() const {
    return _typed_memory ? &(*_typed_memory)[_offset] : nullptr;
  }

  constexpr raw_reference operator*() { return (*_typed_memory)[_offset]; }
  constexpr const_raw_reference operator*() const {
    return (*_typed_memory)[_offset];
  }

  constexpr raw_pointer operator->() { return get(); }

  constexpr bool operator==(const pointer other) const {
    return get() == other.get();
  }

  constexpr bool operator!=(const pointer other) const {
    return get() != other.get();
  }

  constexpr bool operator==(const_raw_pointer raw) const {
    return get() == raw;
  }
  constexpr bool operator!=(const_raw_pointer raw) const {
    return get() != raw;
  }

  constexpr pointer operator+(difference_type offset) const {
    return pointer{*_typed_memory, _offset + offset};
  }

  constexpr difference_type operator-(const pointer &other) const {
    return static_cast<difference_type>(_offset)
         - static_cast<difference_type>(other._offset);
  }

  constexpr pointer &operator++() {
    ++_offset;
    return *this;
  }

  constexpr pointer operator++(int) {
    auto temp = *this;
    ++(*this);
    return temp;
  }

  constexpr typed_memory_reference memory() { return *_typed_memory; }
};

template <typename T> class static_memory_backend {
public:
  using value_type = T;
  using size_type = std::size_t;
  constexpr static size_type _capacity =
      typed_static_memory_size + 1;  // +1 to reserve index 0, null semantics
  using storage_type = T[_capacity]; // +1 to reserve index 0, null semantics
  using reference = T &;
  using const_reference = const T &;

private:
  storage_type _storage;

public:
  constexpr size_type capacity() const noexcept { return _capacity; }
  constexpr reference operator[](std::size_t index) { return _storage[index]; }
  constexpr const_reference operator[](std::size_t index) const {
    return _storage[index];
  }
};

template <typename T> class typed_static_memory {
public:
  using raw_value = T;
  using size_type = std::size_t;
  using this_type = typed_static_memory<raw_value>;
  using backend_type = static_memory_backend<raw_value>;
  using reference = T &;
  using const_reference = const T &;

  using raw_pointer = raw_value *;
  using const_raw_pointer = const raw_pointer;
  using raw_reference = raw_value &;
  using const_raw_reference = const raw_value &;

  using pointer = static_synthetic_pointer<this_type>;
  using const_pointer = const pointer;
  using difference_type = std::ptrdiff_t;

private:
  std::size_t _offset = 1; // reserve offset 0 for null pointers
  backend_type _backend;

public:
  constexpr size_t capacity() const noexcept { return _backend.capacity(); }
  constexpr std::size_t remaining_size() const noexcept {
    return _backend._capacity - _offset;
  }
  constexpr std::size_t allocated_size() const noexcept {
    return _offset
         - 1; // _offset starts at 1, so we subtract 1 to get the actual size
  }

  constexpr typed_static_memory() noexcept : _backend{} {}

  constexpr ~typed_static_memory() noexcept = default;

  constexpr pointer none() noexcept {
    return pointer{*this, 0}; // return null pointer
  }

  constexpr pointer allocate(std::size_t size) {
    if (size == 0) {
      return pointer{*this, 0}; // return null pointer
    }
    if (size > remaining_size()) {
      return pointer{*this, 0}; // return null pointer
    }
    auto old_offset = _offset;
    _offset += size;
    return pointer{*this, old_offset}; // offset 1 to avoid null pointer
  }

  constexpr void deallocate(pointer ptr) noexcept { (void)ptr; }
  constexpr void deallocate(pointer ptr, std::size_t size) noexcept {
    (void)ptr;
    (void)size;
  }

  constexpr reference operator[](std::size_t index) { return _backend[index]; }
  constexpr const_reference operator[](std::size_t index) const {
    return _backend[index];
  }
};

} // namespace ycetl
