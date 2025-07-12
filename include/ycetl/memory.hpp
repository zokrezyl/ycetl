#pragma once
#include <bit>
// #include <cstring>
#include <memory>
// #include <ycetl/allocator_traits.hpp>
#include <ycetl/impl/dynamic_allocator.hpp>
#include <ycetl/impl/multitype_allocator.hpp>

namespace ycetl {

template <typename T> class owned_pointer {
  T *_ptr = nullptr;
  bool _owned = true;

public:
  constexpr owned_pointer() : _ptr(new T) {}
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

template <typename T, typename MultitypeAllocator>
constexpr T *allocate(MultitypeAllocator &alloc, std::size_t n) {
  return alloc.template allocate<T>(n);
}

template <typename TypeSet>
using multitype_dynamic_allocator =
    ::ycetl::memory::multitype_allocator<::ycetl::memory::dynamic_allocator,
                                         TypeSet>;

// TODO thte following template should work both for typeset and for single type
template <typename T>
using default_allocator =
    ::ycetl::memory::multitype_allocator<::ycetl::memory::dynamic_allocator,
                                         ycetl::type_set<T>>;

namespace memory {
#if 0
template <typename InputIt, typename OutputIt, typename Alloc>
constexpr OutputIt uninitialized_move(InputIt first, InputIt last,
                                      OutputIt dest, Alloc &alloc) {
  for (; first != last; ++first, ++dest)
    ycetl::allocator_traits<Alloc>::construct(alloc, std::addressof(*dest),
                                              std::move(*first));
  return dest;
}

template <typename InputIt, typename OutputIt, typename Alloc>
OutputIt uninitialized_copy(InputIt first, InputIt last, OutputIt dest,
                            Alloc &alloc) {
  for (; first != last; ++first, (void)++dest)
    ycetl::allocator_traits<Alloc>::construct(alloc, std::addressof(*dest),
                                              *first);
  return dest;
}

template <typename InputIt, typename OutputIt, typename Alloc>
constexpr OutputIt uninitialized_move_if_noexcept(InputIt first, InputIt last,
                                                  OutputIt dest, Alloc &alloc) {
  for (; first != last; ++first, ++dest)
    ycetl::allocator_traits<Alloc>::construct(alloc, std::addressof(*dest),
                                              std::move_if_noexcept(*first));
  return dest;
}

template <typename ForwardIt, typename Alloc>
constexpr void destroy(ForwardIt first, ForwardIt last, Alloc &alloc) {
  for (; first != last; ++first)
    ycetl::allocator_traits<Alloc>::destroy(alloc, std::addressof(*first));
}

#endif

} // namespace memory
} // namespace ycetl
