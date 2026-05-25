// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <initializer_list>
#include <utility>

namespace ycetl {

// Fixed-capacity, variable-size sequence container — std::inplace_vector
// (C++26) / boost::container::static_vector look-alike. All storage lives
// inside the object itself: no heap, no typed_memory backend.
//
// Why this exists alongside ycetl::array<T, N> and ycetl::vector<T>:
//
//   - array<T, N> always carries exactly N elements. For a sequence whose
//     length varies up to N at compile time, you have to pad the tail and
//     carry a separate count alongside.
//   - vector<T> grows on a heap (typed_memory) — wrong shape for the
//     result-memory side of the yce machine, which wants its storage to
//     live in .rodata as part of a baked constexpr value.
//   - inplace_vector<T, N> is the missing shape: capacity N baked into
//     the type, current size tracked, all storage in-place. Drop one
//     into a constexpr result struct and it ships in .rodata.
//
// T must be default-constructible (matches the existing ycetl::array
// constraint). Slots past _size hold default-constructed values; pop_back
// / clear / resize-down only adjust _size — they don't run T's destructor
// per call. T's destructor runs once per slot when the inplace_vector
// itself is destroyed.

template <typename T, std::size_t N> class inplace_vector {
public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using iterator = T *;
  using const_iterator = const T *;

private:
  T _storage[N]{};
  size_type _size = 0;

public:
  constexpr inplace_vector() noexcept = default;

  constexpr inplace_vector(size_type n) {
    for (size_type i = 0; i < n; ++i)
      _storage[i] = T{};
    _size = n;
  }

  constexpr inplace_vector(size_type n, const T &v) {
    for (size_type i = 0; i < n; ++i)
      _storage[i] = v;
    _size = n;
  }

  constexpr inplace_vector(std::initializer_list<T> il) {
    for (const auto &x : il)
      _storage[_size++] = x;
  }

  // capacity
  static constexpr size_type capacity() noexcept { return N; }
  static constexpr size_type max_size() noexcept { return N; }
  constexpr size_type size() const noexcept { return _size; }
  constexpr bool empty() const noexcept { return _size == 0; }
  constexpr bool full() const noexcept { return _size == N; }

  // element access
  constexpr reference operator[](size_type i) noexcept { return _storage[i]; }
  constexpr const_reference operator[](size_type i) const noexcept {
    return _storage[i];
  }
  constexpr reference front() noexcept { return _storage[0]; }
  constexpr const_reference front() const noexcept { return _storage[0]; }
  constexpr reference back() noexcept { return _storage[_size - 1]; }
  constexpr const_reference back() const noexcept {
    return _storage[_size - 1];
  }
  constexpr pointer data() noexcept { return _storage; }
  constexpr const_pointer data() const noexcept { return _storage; }

  // iterators
  constexpr iterator begin() noexcept { return _storage; }
  constexpr iterator end() noexcept { return _storage + _size; }
  constexpr const_iterator begin() const noexcept { return _storage; }
  constexpr const_iterator end() const noexcept { return _storage + _size; }
  constexpr const_iterator cbegin() const noexcept { return _storage; }
  constexpr const_iterator cend() const noexcept { return _storage + _size; }

  // modifiers
  template <typename... Args> constexpr reference emplace_back(Args &&...args) {
    _storage[_size] = T(std::forward<Args>(args)...);
    return _storage[_size++];
  }
  constexpr void push_back(const T &v) { _storage[_size++] = v; }
  constexpr void push_back(T &&v) { _storage[_size++] = std::move(v); }
  constexpr void pop_back() noexcept { --_size; }
  constexpr void clear() noexcept { _size = 0; }

  constexpr void resize(size_type n) {
    if (n > _size)
      for (size_type i = _size; i < n; ++i)
        _storage[i] = T{};
    _size = n;
  }
  constexpr void resize(size_type n, const T &v) {
    if (n > _size)
      for (size_type i = _size; i < n; ++i)
        _storage[i] = v;
    _size = n;
  }

  constexpr bool operator==(const inplace_vector &o) const {
    if (_size != o._size)
      return false;
    for (size_type i = 0; i < _size; ++i)
      if (!(_storage[i] == o._storage[i]))
        return false;
    return true;
  }
};

} // namespace ycetl
