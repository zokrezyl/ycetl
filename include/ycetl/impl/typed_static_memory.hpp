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
  using value_type = typename typed_memory::value_type;
  using this_type = static_synthetic_pointer<typed_memory>;
  using pointer = value_type *;
  using const_pointer = value_type *;
  using reference = typename typed_memory::reference;
  using const_reference = typename typed_memory::const_reference;

private:
  typed_memory_reference _typed_memory;
  std::size_t _offset = 1; // reserve offset 0 as null
public:
  constexpr static_synthetic_pointer(typed_memory &mem, std::size_t offset)
      : _typed_memory(mem), _offset(offset) {}

  constexpr reference operator[](std::size_t index) {
    return _typed_memory[_offset + index - 1];
  }
  constexpr const_reference operator[](std::size_t index) const {
    return _typed_memory[_offset + index - 1];
  }
  constexpr auto operator+(std::size_t offset) const {
    return static_synthetic_pointer{_typed_memory, _offset + offset};
  }
  constexpr pointer get() { return &_typed_memory[_offset - 1]; }
  constexpr const_pointer get() const { return &_typed_memory[_offset - 1]; }

  constexpr pointer operator*() { return _typed_memory[_offset - 1]; }

  constexpr pointer operator->() { return &_typed_memory[_offset - 1]; }

  typed_memory_reference memory() { return _typed_memory; }
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
  using value_type = T;
  using size_type = std::size_t;
  using this_type = typed_static_memory<value_type>;
  using backend_type = static_memory_backend<value_type>;
  using reference = T &;
  using const_reference = const T &;
  using pointer = static_synthetic_pointer<this_type>;
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
    return _offset -
           1; // _offset starts at 1, so we subtract 1 to get the actual size
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
