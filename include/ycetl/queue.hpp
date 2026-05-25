// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <utility>

#include <ycetl/deque.hpp>

namespace ycetl {

// FIFO adapter on top of ycetl::deque. Same interface contract as
// std::queue: push at the back, pop from the front, front() peeks at
// what would be popped next.
//
// The Container template parameter stays in the signature for symmetry
// with std::queue so a caller can swap in their own backing (e.g.
// ycetl::list for iterator stability across grow). Anything that
// exposes push_back / pop_front / front / back / size / empty works.
template <typename T, typename Container = deque<T>> class queue {
public:
  using container_type = Container;
  using value_type = T;
  using size_type = std::size_t;
  using reference = T &;
  using const_reference = const T &;

private:
  container_type _c;

public:
  constexpr queue() = default;
  explicit constexpr queue(container_type c) : _c(std::move(c)) {}

  constexpr bool empty() const { return _c.empty(); }
  constexpr size_type size() const { return _c.size(); }

  constexpr reference front() { return _c.front(); }
  constexpr const_reference front() const { return _c.front(); }
  constexpr reference back() { return _c.back(); }
  constexpr const_reference back() const { return _c.back(); }

  constexpr void push(const T &v) { _c.push_back(v); }
  constexpr void push(T &&v) { _c.push_back(std::move(v)); }

  template <typename... Args> constexpr void emplace(Args &&...args) {
    _c.push_back(T(std::forward<Args>(args)...));
  }

  constexpr void pop() { _c.pop_front(); }

  constexpr container_type &container() noexcept { return _c; }
  constexpr const container_type &container() const noexcept { return _c; }
};

} // namespace ycetl
