// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <utility>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

// LIFO adapter on top of dynamic_array. Same constructor shape as
// dynamic_array itself: optionally takes a memory backend (defaults to
// the per-T typed_dynamic_memory). The Container template parameter
// stays in the signature for symmetry with std::stack so a caller can
// swap in their own backing store (e.g. dynamic_array<T,
// typed_static_memory<T>>) for a fixed-size stack.
template <typename T, typename Container = dynamic_array<T>> class stack {
public:
  using container_type = Container;
  using value_type = T;
  using size_type = std::size_t;
  using reference = T &;
  using const_reference = const T &;

private:
  container_type _c;

public:
  constexpr stack() = default;

  explicit constexpr stack(container_type c) : _c(std::move(c)) {}

  template <typename Memory> explicit constexpr stack(Memory &m) : _c(m) {}

  constexpr bool empty() const { return _c.size() == 0; }
  constexpr size_type size() const { return _c.size(); }

  constexpr reference top() { return _c[_c.size() - 1]; }
  constexpr const_reference top() const { return _c[_c.size() - 1]; }

  constexpr void push(const T &v) { _c.push_back(v); }
  constexpr void push(T &&v) { _c.push_back(std::move(v)); }

  template <typename... Args> constexpr void emplace(Args &&...args) {
    _c.push_back(T(std::forward<Args>(args)...));
  }

  constexpr void pop() { _c.pop_back(); }

  constexpr void swap(stack &other) noexcept { std::swap(_c, other._c); }

  constexpr container_type &container() noexcept { return _c; }
  constexpr const container_type &container() const noexcept { return _c; }
};

} // namespace ycetl
