#pragma once

#include <array>
#include <bit>
#include <cstddef>

#include <ycetl/cstring.hpp>

// simple constexpr execution tracer
// simplest is to attache to the allocator
namespace ycetl {
// clang-format off
//
//
namespace trace {

template <typename T>
constexpr void pack(char *buffer, const T &value) {
  if constexpr (std::is_pointer_v<T>) {
    pack(buffer, *value);
    return;
  }
  auto bytes = std::bit_cast<std::array<char, sizeof(T)>>(value);
  for (std::size_t i = 0; i < sizeof(T); ++i)
    buffer[i] = bytes[i];
}

template <>
constexpr void pack<const char *>(char *buffer, const char *const &value) {
  if (value == nullptr) {
    pack(buffer, static_cast<std::size_t>(0));
    return;
  }
  std::size_t len = ycetl::strlen(value);

  for (std::size_t i = 0; i < len; ++i) {
    buffer[i] = static_cast<char>(value[i]);
  }
}

class TracerMemory {
  char *_memory = nullptr;
  std::size_t _size = 0;
  std::size_t _offset = 0;
  std::array<char*, 1000> _pointers;
  bool _owns_memory = true;
  std::size_t slots_allocated = 0;

public:
  // lazy initialization
  constexpr TracerMemory(): _memory(nullptr), _size(0), _owns_memory(true) {

  }

  constexpr TracerMemory(char *memory, std::size_t size)
      : _memory(memory), _size(size) {
    if (_memory == nullptr || _size == 0) {
      // TODO: throw exception
    }
  }

 constexpr  char *get_memory(std::size_t _size) {
    if (_memory == nullptr || _size == 0) {
      return nullptr; // TODO: throw exception
    }
    char *ptr = _memory + _offset;
    _offset += _size;
    return ptr;
  }

  // destructor
 constexpr  ~TracerMemory() {
    if (_owns_memory) {
      for (std::size_t i = 0; i < slots_allocated; ++i) {
        if (_pointers[i] != nullptr) {
          delete[] _pointers[i];
        }
      }
    }
  }

  constexpr std::size_t consumed() const {
    return _offset;
  }

};

template <typename T>
class TraceMessage {
  char *_buffer = nullptr;
  std::size_t _size = 0;
  std::size_t _offset = 0;
public:
  constexpr TraceMessage(T) {

  }
};

class Tracer {
  TracerMemory *_memory = nullptr;
public:
  constexpr Tracer(): _memory(new TracerMemory()) {

  }

  constexpr Tracer(TracerMemory *memory) : _memory(memory) {

  }

  constexpr void trace(const char *value) {
    std::size_t len = ycetl::strlen(value) + 1;
    char *buffer = _memory->get_memory(len);
    pack(buffer, value);
    buffer[len-1] = '\n'; // ensure null termination
    buffer[len] = '\0'; // ensure null termination
  }
};

}
// clang-format on
} // namespace ycetl
