// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace ycetl {

inline constexpr std::size_t dynamic_extent = static_cast<std::size_t>(-1);

// Non-owning view over a contiguous sequence. No allocator, no Memory
// backend — a span is two values: pointer + length. Designed for
// constexpr signatures: handing a sub-range of a baked-in std::array (or
// of a ycetl::dynamic_array's storage) around without copying.
//
// Subset of std::span: dynamic-extent only, no static-extent
// specialisation. That's deliberate — the static-extent overload
// triples the surface area and we have no use case for it yet.
template <typename T> class span {
public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using iterator = T *;
  using const_iterator = const T *;

private:
  pointer _data = nullptr;
  size_type _size = 0;

public:
  constexpr span() noexcept = default;
  constexpr span(pointer p, size_type n) noexcept : _data(p), _size(n) {}
  constexpr span(pointer first, pointer last) noexcept
      : _data(first), _size(static_cast<size_type>(last - first)) {}

  template <std::size_t N>
  constexpr span(T (&arr)[N]) noexcept : _data(arr), _size(N) {}

  template <std::size_t N>
  constexpr span(std::array<value_type, N> &arr) noexcept
      : _data(arr.data()), _size(N) {}

  template <std::size_t N>
  constexpr span(const std::array<value_type, N> &arr) noexcept
    requires std::is_const_v<T>
      : _data(arr.data()), _size(N) {}

  constexpr pointer data() const noexcept { return _data; }
  constexpr size_type size() const noexcept { return _size; }
  constexpr size_type size_bytes() const noexcept { return _size * sizeof(T); }
  constexpr bool empty() const noexcept { return _size == 0; }

  constexpr reference operator[](size_type i) const noexcept {
    return _data[i];
  }
  constexpr reference front() const noexcept { return _data[0]; }
  constexpr reference back() const noexcept { return _data[_size - 1]; }

  constexpr iterator begin() const noexcept { return _data; }
  constexpr iterator end() const noexcept { return _data + _size; }

  constexpr span first(size_type n) const noexcept { return {_data, n}; }
  constexpr span last(size_type n) const noexcept {
    return {_data + (_size - n), n};
  }
  constexpr span subspan(size_type offset,
                         size_type count = dynamic_extent) const noexcept {
    return {_data + offset, count == dynamic_extent ? _size - offset : count};
  }
};

// Deduction guides — let `span s{ptr, n}` and `span s{arr}` work without
// spelling the element type.
template <typename T, std::size_t N> span(T (&)[N]) -> span<T>;
template <typename T, std::size_t N> span(std::array<T, N> &) -> span<T>;
template <typename T, std::size_t N>
span(const std::array<T, N> &) -> span<const T>;
template <typename T> span(T *, std::size_t) -> span<T>;
template <typename T> span(T *, T *) -> span<T>;

} // namespace ycetl
