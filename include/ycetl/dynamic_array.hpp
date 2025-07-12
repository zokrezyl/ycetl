#pragma once
#include <cstddef>
#include <new>
#include <utility>
#include <ycetl/memory.hpp> // allocate<T>

namespace ycetl {

template <typename T> class dynamic_array {
  T *_data = nullptr;
  std::size_t _size = 0;
  std::size_t _capacity = 0;

  template <class... Args>
  constexpr void construct(std::size_t i, Args &&...args) {
    std::construct_at(_data + i, std::forward<Args>(args)...);
  }

  constexpr void destroy_range(std::size_t first, std::size_t last) noexcept {
    for (std::size_t i = first; i < last; ++i)
      std::destroy_at(_data + i);
  }

public:
  constexpr dynamic_array() = default;

  template <class Alloc> constexpr explicit dynamic_array(Alloc &) {}

  /* sized construction --------------------------------------------------- */
  template <class Alloc>
  constexpr dynamic_array(Alloc &a, std::size_t n)
      : _data(allocate<T>(a, n)), _size(n), _capacity(n) {
    for (std::size_t i = 0; i < n; ++i)
      construct(i);
  }

  template <class Alloc>
  constexpr dynamic_array(Alloc &a, std::size_t n, const T &v)
      : _data(allocate<T>(a, n)), _size(n), _capacity(n) {
    for (std::size_t i = 0; i < n; ++i)
      construct(i, v);
  }

  template <class Alloc, class It>
  constexpr dynamic_array(Alloc &a, It first, std::size_t n)
      : _data(allocate<T>(a, n)), _size(n), _capacity(n) {
    for (std::size_t i = 0; i < n; ++i)
      construct(i, *(first + i));
  }

  /* move semantics ------------------------------------------------------- */
  constexpr dynamic_array(dynamic_array &&other) noexcept
      : _data(std::exchange(other._data, nullptr)),
        _size(std::exchange(other._size, 0)),
        _capacity(std::exchange(other._capacity, 0)) {}

  constexpr dynamic_array &operator=(dynamic_array &&other) noexcept {
    if (this != &other) {
      clear();
      _data = std::exchange(other._data, nullptr);
      _size = std::exchange(other._size, 0);
      _capacity = std::exchange(other._capacity, 0);
    }
    return *this;
  }

  dynamic_array(const dynamic_array &) = delete;
  dynamic_array &operator=(const dynamic_array &) = delete;

  /* destructor ----------------------------------------------------------- */
  constexpr ~dynamic_array() { clear(); }

  /* capacity ------------------------------------------------------------- */
  constexpr std::size_t size() const { return _size; }
  constexpr std::size_t capacity() const { return _capacity; }

  /* element access ------------------------------------------------------- */
  constexpr T &operator[](std::size_t i) { return _data[i]; }
  constexpr const T &operator[](std::size_t i) const { return _data[i]; }

  constexpr T *begin() { return _data; }
  constexpr T *end() { return _data + _size; }
  constexpr const T *begin() const { return _data; }
  constexpr const T *end() const { return _data + _size; }

  /* reserve -------------------------------------------------------------- */
  template <class Alloc> constexpr void reserve(Alloc &a, std::size_t new_cap) {
    if (new_cap <= _capacity)
      return;
    T *new_buf = allocate<T>(a, new_cap);
    for (std::size_t i = 0; i < _size; ++i)
      std::construct_at(new_buf + i, std::move(_data[i]));
    destroy_range(0, _size); // destroy moved‑from
    _data = new_buf;
    _capacity = new_cap;
  }

  /* resize --------------------------------------------------------------- */
  template <class Alloc> constexpr void resize(Alloc &a, std::size_t new_size) {
    if (new_size < _size) {
      destroy_range(new_size, _size);
      _size = new_size;
    } else if (new_size > _size) {
      if (new_size > _capacity)
        reserve(a, new_size);
      for (; _size < new_size; ++_size)
        construct(_size);
    }
  }

  /* modifiers ------------------------------------------------------------ */
  template <class Alloc, class... Args>
  constexpr T *emplace_back(Alloc &a, Args &&...args) {
    if (_size == _capacity)
      reserve(a, _capacity ? _capacity * 2 : 4);
    construct(_size, std::forward<Args>(args)...);
    return _data + _size++;
  }

  template <class Alloc> constexpr void push_back(const T &v, Alloc &a) {
    emplace_back(a, v);
  }

  template <class Alloc> constexpr void push_back(T &&v, Alloc &a) {
    emplace_back(a, std::move(v));
  }

  constexpr void pop_back() {
    --_size;
    std::destroy_at(_data + _size);
  }

  constexpr void clear() noexcept {
    destroy_range(0, _size);
    _size = 0;
  }

  /* insert --------------------------------------------------------------- */
  template <class Alloc> constexpr T *insert(Alloc &a, T *pos, const T &v) {
    std::size_t idx = pos - _data;
    if (_size == _capacity)
      reserve(a, _capacity ? _capacity * 2 : 4);
    for (std::size_t i = _size; i > idx; --i)
      std::construct_at(_data + i, std::move(_data[i - 1]));
    ++_size;
    _data[idx] = v;
    return _data + idx;
  }
};

} // namespace ycetl
