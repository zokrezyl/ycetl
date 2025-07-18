#pragma once
#include <bit>
// #include <cstring>
#include <memory>
// #include <ycetl/allocator_traits.hpp>
#include <ycetl/impl/multitype_memory.hpp>
#include <ycetl/impl/typed_dynamic_memory.hpp>
#include <ycetl/impl/typed_static_memory.hpp>
#include <ycetl/trivial_shared_ptr.hpp>
#include <ycetl/types.hpp>

namespace ycetl {

template <typename TypedMemoryType>
class typed_memory_shared_ptr : public trivial_shared_ptr<TypedMemoryType> {
public:
  using typed_memory_type = TypedMemoryType;
  using stored_type = typed_memory_type::stored_type;

  constexpr stored_type *allocate(std::size_t n) {
    return this->get()->allocate(n);
  }

  constexpr void deallocate(stored_type *p, std::size_t n) {
    this->get()->deallocate(p, n);
  }
};

template <typename T> class owned_pointer {
  T *_ptr = nullptr;
  bool _owned = true;

public:
  constexpr owned_pointer() : _ptr(new T()) {}
  template <typename... Args>
  constexpr owned_pointer(Args &&...args)
      : _ptr(new T(std::forward<Args>(args)...)) {}
  constexpr owned_pointer(T *p) : _ptr(p), _owned(false) {}
  owned_pointer(const owned_pointer &) = delete;
  owned_pointer &operator=(const owned_pointer &) = delete;

  constexpr owned_pointer(owned_pointer &&other) noexcept
      : _ptr(other._ptr), _owned(other._owned) {
    other._ptr = nullptr;
    other._owned = false;
  }

  constexpr owned_pointer &operator=(owned_pointer &&other) noexcept {
    if (this != &other) {
      if (_owned && _ptr)
        delete _ptr;
      _ptr = other._ptr;
      _owned = other._owned;
      other._ptr = nullptr;
      other._owned = false;
    }
    return *this;
  }

  constexpr ~owned_pointer() {
    if (_owned && _ptr)
      delete _ptr; // ← honour non‑owned case
  }
  constexpr T *get() { return _ptr; }
  constexpr const T *get() const { return _ptr; }
  constexpr T &operator*() { return *_ptr; }
  constexpr const T &operator*() const { return *_ptr; }

  constexpr T *operator->() { return _ptr; }
  constexpr const T *operator->() const { return _ptr; }
};

template <typename T, typename MultitypeMemory>
constexpr T *allocate(MultitypeMemory &alloc, std::size_t n) {
  return alloc.template allocate<T>(n);
}

template <typename T, typename MultitypeMemory>
constexpr void deallocate(MultitypeMemory &alloc, T *ptr, std::size_t n) {
  alloc.template deallocate<T>(ptr, n);
}

template <typename T>
using typed_shared_dynamic_memory =
    typed_memory_shared_ptr<typed_dynamic_memory<T>>;

template <typename... RawType>
using dynamic_memory =
    multitype_memory<typed_shared_dynamic_memory, RawType...>;

template <typename... RawType>
using default_memory = dynamic_memory<RawType...>;

template <typename... RawType>
using static_memory = multitype_memory<typed_static_memory, RawType...>;

namespace memory {} // namespace memory
} // namespace ycetl
