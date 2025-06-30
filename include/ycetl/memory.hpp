#pragma once
#include <bit>
#include <cstring>
#include <type_traits>
#include <ycetl/cstddef.hpp>
#include <ycetl/trivial_array.hpp>

namespace ycetl {

template <typename T>
constexpr bool has_likely_initialized_instances =
    not(std::is_trivially_default_constructible_v<T> &&
        std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T>);

// packs
template <typename BufferT, typename T>
constexpr void pack(BufferT *buffer, const T value) {
  static_assert(std::is_trivially_copyable_v<T>);
#if 0
  static_assert(false, "fix this, it is not working yet");
  // TODO: the target should be the BufferT::value_type
  auto bytes = std::bit_cast<std::array<BufferT, sizeof(T) / sizeof(BufferT)>>(value);
  for (yectl::size_t i = 0; i < sizeof(T); ++i)
    buffer[i] = bytes[i];
#endif
}

template <typename BufferT, typename T>
constexpr void unpack(BufferT *buffer, T *value) {
  static_assert(std::is_trivially_copyable_v<T>);
#if 0
  std::array<BufferT, sizeof(T) / sizeof(BufferT)> bytes{};

  for (yectl::size_t i = 0; i < sizeof(T); ++i)
    bytes[i] = buffer[i];

  *value = std::bit_cast<T>(bytes);
#endif
}

// the heap arena is an opaque type that implements allocation around a piece of
// memory allocated on the heap with new expression as per rules of constexpr
// such an object shall be created on stack and the destructor will be called
// the memory behind can be used in the subsequent executions of recursivel
// called constexpr
#if 0
template <size_t N = 100 * 1024 * 1024> class _heap_arena {
public:
  size_t *_memory;
  yectl::size_t _size;
  yectl::size_t _offset;
  constexpr void *allocate(yectl::size_t size) {
    if (_offset + size > _size) {
      // throw "aoeu";
      return nullptr; // or throw an exception
    }
    void *ptr = _memory + _offset;
    _offset += size;
    return ptr;
  }
  constexpr void heap_arena()
      : _offset(0), _size(N), _memory(new size_t[N / sizeof(size_t)]) {
    //_memory = new size_t[N / sizeof(size_t)];
    //_size = N;
    //_offset = 0;
    static_assert(N > 0, "Arena size must be greater than 0");
  }

  constexpr exposed_arena &expose() {
    return *static_cast<exposed_arena *>(this);
  }
  constexpr ~heap_arena() { delete[] _memory; }
};
#endif

class heap_arena {
public:
  unsigned char *_memory;
  ycetl::size_t _size;
  ycetl::size_t _offset;

  constexpr heap_arena(ycetl::size_t _size)
      : _offset(0), _size(_size),
        _memory(_size > 0 ? new unsigned char[_size] : nullptr) {
    //_memory = new size_t[N / sizeof(size_t)];
    //_size = N;
    //_offset = 0;
  }
  constexpr ~heap_arena() { delete[] _memory; }

  constexpr void *memory() { return static_cast<void *>(_memory); }
  constexpr ycetl::size_t size() const { return _size; }
};

// the rom arena is a fixed size arena that will be mapped at runtime
// into the .rodata section of the program
// great care shall be taken where  the rom_arena instance is created
// eg:
//
// class A {
// rome_arena<100 * 1024 * 1024> _arena;
//
// };
//
// constexpr A a;
//
// the arena will be created in the .rodata on "heap", thus the size limitation
// is less strict (see the .rodata section in the linker script)
//
// if instead you are declaring the rom_arena in a function, it will be created
// on the stack and thus the size limitation is strict, arround 20 MB (again see
// the documentation for the .rodata section in the linker script)
//
//
//
//
// by design the template argument is supposed to be the size in bytes
//
template <std::size_t SizeInBytes> struct rom_arena {
  trivial_array<unsigned char, SizeInBytes> _data;

public:
  constexpr rom_arena() : _data() {};
  constexpr void *memory() { return static_cast<void *>(_data.data()); }
  constexpr ycetl::size_t size() const { return SizeInBytes; }
  constexpr ~rom_arena() = default;
};

// the alloccator has duality
// it can either work with the memory that is passed in as constructor argument
// or create it's own heap_arena instance
class _Allocator {
private:
  heap_arena *_heap_arena;
  void *_memory;
  size_t _size;

public:
  constexpr _Allocator(void *memory, size_t _size)
      : _heap_arena(nullptr), _memory(memory), _size(_size) {}

  constexpr _Allocator(size_t _size = 0)
      : _heap_arena(new heap_arena(_size)), _memory(_heap_arena->memory()),
        _size(_heap_arena->size()) {}

  constexpr void *allocate(ycetl::size_t size) { return nullptr; }
  // destructor
  constexpr ~_Allocator() {
    if (_heap_arena) {
      delete _heap_arena; // deallocate the heap arena
    }
  }
};

struct _SharedPtrControlBlock {
  ycetl::size_t _ref_count;

public:
  constexpr _SharedPtrControlBlock() : _ref_count(0) {}
  constexpr void increment() { ++_ref_count; }
  constexpr void decrement() { --_ref_count; }
  constexpr ycetl::size_t ref_count() const { return _ref_count; }
};

// behaves like a smart pointer, but it is not a smart pointer
class Allocator {
private:
  _Allocator *_allocator;
  _SharedPtrControlBlock *_control_block;

public:
  // if no allocator is passed, it will create a default one
  constexpr Allocator()
      : _allocator(new _Allocator()),
        _control_block(new _SharedPtrControlBlock()) {}

  constexpr Allocator(_Allocator *_allocator)
      : _allocator(_allocator), _control_block(nullptr) {}

  // destructor
  constexpr ~Allocator() {
    if (_control_block) {
      // decrement the reference count
      _control_block->decrement();
      if (_control_block->ref_count() == 0) {
        delete _control_block; // deallocate the control block
        delete _allocator;     // deallocate the allocator
      }
    }
    // else .. we do not own the allocator, so we do not delete it
  }

  constexpr Allocator(const Allocator &other)
      : _allocator(other._allocator), _control_block(other._control_block) {
    if (_control_block) {
      // increment the reference count
      _control_block->increment();
    }
  }

  constexpr ycetl::size_t ref_count() const {
    if (_control_block) {
      return _control_block->ref_count();
    } else {
      // if there is no control block, we do not have a reference count
      return 0;
    }
  }

  constexpr _Allocator *allocator() { return _allocator; }

  constexpr void *allocate(ycetl::size_t size) {
    if (_allocator) {
      return _allocator->allocate(size);
    }
    return nullptr; // or throw an exception
  }
};

/*
template <std::size_t N> class RomAllocatorFactory {
public:
  rom_arena<N> _arena;
  Allocator _allocator;
  constexpr RomAllocatorFactory()
      : _arena(), _allocator(_arena.memory(), _arena.size()) {}

  constexpr Allocator *allocator() { return &_allocator; }

  constexpr ycetl::size_t size() const { return _arena.size(); }
};
*/

} // namespace ycetl
