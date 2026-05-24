#pragma once
#include <cstddef>
#include <cstdint> // uintptr_t
#include <memory>

namespace ycetl {
namespace memory {

// we assume that source is a sequence of source bytes
template <typename Target, typename Source>
constexpr inline std::size_t align(Target *&target, Source *source) noexcept {}

// generic memry  holding structure
// that holds a pointer to a memory block of type T
// the size of the memory block and
// an offset to next available memory
// note that all pointer arithmetic is done in terms of bytes
// this allows to compact allocation if T would have a large size
template <typename T> struct memory_struct {
  T *ptr = nullptr;       // pointer to the memory blockT *
  std::size_t size = 0;   // size of the memory block in bytes
  std::size_t offset = 0; // offset to next available memory in  bytes
public:
  using value_type = T; // type alias for the value type
  memory_struct(T *p, std::size_t s) : ptr(p), size(s), offset(0) {}
};

template <typename T, std::size_t N> struct static_memory_struct {
  T ptr[N];               // pointer to the memory block of type T
  std::size_t size = N;   // size of the memory block in bytes
  std::size_t offset = 0; // offset to next available memory in  bytes
public:
  using value_type = T; // type alias for the value type
  static_memory_struct() = default;
};

// simple allocator function that allocates memory
// from a memory_struct of type From
// for a type To
// the memory is alligned to the size of To
template <typename To, typename Memory> constexpr To *allocate(Memory &mem) {
  using From = typename Memory::value_type;
  // first we calculate the remaining size in the memory block
  std::size_t remaining_size = mem.size - mem.offset;
  // we use std::align to align the memory to the size of To
  std::size_t alignment = alignof(To);
  From *aligned_to_from_ptr = mem.ptr + mem.offset;
  void *aligned_ptr = static_cast<void *>(aligned_to_from_ptr);
  void *aligned_to_to_ptr = align(alignment, sizeof(To), aligned_ptr,
                                  remaining_size);
  // write back the offset
  mem.offset = remaining_size;
  return static_cast<To *>(aligned_to_to_ptr);
};

} // namespace memory
} // namespace ycetl
