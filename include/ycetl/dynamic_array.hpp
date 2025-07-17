#pragma once
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>

#include <ycetl/memory.hpp>

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
  template <bool IsConst> class basic_iterator {
    friend class dynamic_array;

    T *_ptr;

    using pointer_type = std::conditional_t<IsConst, const T *, T *>;
    using reference_type = std::conditional_t<IsConst, const T &, T &>;

  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = pointer_type;
    using reference = reference_type;

    constexpr basic_iterator(T *p = nullptr) noexcept : _ptr(p) {}

    operator basic_iterator<true>() const noexcept
      requires(!IsConst)
    {
      return basic_iterator<true>(_ptr);
    }

    constexpr reference operator*() const noexcept { return *_ptr; }
    constexpr pointer operator->() const noexcept { return _ptr; }

    constexpr basic_iterator &operator++() noexcept {
      ++_ptr;
      return *this;
    }
    constexpr basic_iterator operator++(int) noexcept {
      basic_iterator temp = *this;
      ++(*this);
      return temp;
    }

    constexpr basic_iterator &operator--() noexcept {
      --_ptr;
      return *this;
    }
    constexpr basic_iterator operator--(int) noexcept {
      basic_iterator temp = *this;
      --(*this);
      return temp;
    }

    constexpr basic_iterator &operator+=(difference_type n) noexcept {
      _ptr += n;
      return *this;
    }
    constexpr basic_iterator &operator-=(difference_type n) noexcept {
      _ptr -= n;
      return *this;
    }

    constexpr basic_iterator operator+(difference_type n) const noexcept {
      return basic_iterator(_ptr + n);
    }
    constexpr basic_iterator operator-(difference_type n) const noexcept {
      return basic_iterator(_ptr - n);
    }

    constexpr difference_type
    operator-(const basic_iterator &other) const noexcept {
      return _ptr - other._ptr;
    }

    constexpr bool operator==(const basic_iterator &other) const noexcept {
      return _ptr == other._ptr;
    }
    constexpr bool operator!=(const basic_iterator &other) const noexcept {
      return _ptr != other._ptr;
    }
    constexpr bool operator<(const basic_iterator &other) const noexcept {
      return _ptr < other._ptr;
    }
    constexpr bool operator>(const basic_iterator &other) const noexcept {
      return _ptr > other._ptr;
    }
    constexpr bool operator<=(const basic_iterator &other) const noexcept {
      return _ptr <= other._ptr;
    }
    constexpr bool operator>=(const basic_iterator &other) const noexcept {
      return _ptr >= other._ptr;
    }

    constexpr reference operator[](difference_type n) const noexcept {
      return _ptr[n];
    }
  };

  using iterator = basic_iterator<false>;
  using const_iterator = basic_iterator<true>;

  constexpr iterator begin() noexcept { return iterator(_data); }
  constexpr iterator end() noexcept { return iterator(_data + _size); }
  constexpr T *data() noexcept { return _data; }

  constexpr const_iterator begin() const noexcept {
    return const_iterator(_data);
  }
  constexpr const_iterator end() const noexcept {
    return const_iterator(_data + _size);
  }

  constexpr const_iterator cbegin() const noexcept {
    return const_iterator(_data);
  }
  constexpr const_iterator cend() const noexcept {
    return const_iterator(_data + _size);
  }

  // Re-enabled the default constructor
  constexpr dynamic_array() noexcept {}

  template <class Memory> constexpr explicit dynamic_array(Memory &) noexcept {}

  template <class Memory>
  constexpr dynamic_array(Memory &a, std::size_t n)
      : _data(ycetl::allocate<T>(a, n)), _size(n), _capacity(n) {
    for (std::size_t i = 0; i < n; ++i)
      construct(i);
  }

  template <class Memory>
  constexpr dynamic_array(Memory &a, std::size_t n, const T &v)
      : _data(ycetl::allocate<T>(a, n)), _size(n), _capacity(n) {
    for (std::size_t i = 0; i < n; ++i)
      construct(i, v);
  }

  template <class Memory, class It>
  constexpr dynamic_array(Memory &a, It first, std::size_t n)
    requires std::is_base_of_v<
                 std::input_iterator_tag,
                 typename std::iterator_traits<It>::iterator_category>
      : _data(ycetl::allocate<T>(a, n)), _size(n), _capacity(n) {
    for (std::size_t i = 0; i < n; ++i)
      construct(i, *(first + i));
  }

  template <class Memory>
  constexpr dynamic_array(Memory &a, const dynamic_array &other)
      : dynamic_array(a, other.begin(), other.size()) {}

  template <class Memory>
  constexpr dynamic_array(Memory &a, std::initializer_list<T> il)
      : dynamic_array(a, il.begin(), il.size()) {}

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

  constexpr ~dynamic_array() { clear(); }

  constexpr std::size_t size() const { return _size; }
  constexpr std::size_t capacity() const { return _capacity; }

  constexpr T &operator[](std::size_t i) { return _data[i]; }
  constexpr const T &operator[](std::size_t i) const { return _data[i]; }

  template <class Memory>
  constexpr void reserve(Memory &a, std::size_t new_cap) {
    if (new_cap <= _capacity)
      return;
    T *new_buf = ycetl::allocate<T>(a, new_cap);
    for (std::size_t i = 0; i < _size; ++i)
      std::construct_at(new_buf + i, std::move(_data[i]));
    destroy_range(0, _size);
    if (_data) {
      // Changed to call a.template deallocate<T>(_data)
      a.template deallocate<T>(_data);
    }
    _data = new_buf;
    _capacity = new_cap;
  }

  template <class Memory>
  constexpr void resize(Memory &a, std::size_t new_size) {
    if (new_size < _size) {
      destroy_range(new_size, _size);
    } else if (new_size > _size) {
      if (new_size > _capacity) {
        reserve(a, new_size);
      }
      for (std::size_t i = _size; i < new_size; ++i) {
        construct(i);
      }
    }
    _size = new_size;
  }

  template <class Memory>
  constexpr void resize(Memory &a, std::size_t new_size, const T &v) {
    if (new_size < _size) {
      destroy_range(new_size, _size);
    } else if (new_size > _size) {
      if (new_size > _capacity) {
        reserve(a, new_size);
      }
      for (std::size_t i = _size; i < new_size; ++i) {
        construct(i, v);
      }
    }
    _size = new_size;
  }

  template <class Memory, class... Args>
  constexpr T *emplace_back(Memory &a, Args &&...args) {
    if (_size == _capacity)
      reserve(a, _capacity ? _capacity * 2 : 4);
    construct(_size, std::forward<Args>(args)...);
    return _data + _size++;
  }

  template <class Memory> constexpr void push_back(Memory &a, const T &v) {
    emplace_back(a, v);
  }

  template <class Memory> constexpr void push_back(Memory &a, T &&v) {
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

  template <class Memory>
  constexpr void clear_and_deallocate_buffer(Memory &a) noexcept {
    clear();
    if (_data) {
      // Changed to call a.template deallocate<T>(_data)
      a.template deallocate<T>(_data);
      _data = nullptr;
      _capacity = 0;
    }
  }

  template <class Memory>
  constexpr iterator insert(Memory &a, iterator pos, const T &v) {
    std::size_t idx = pos - begin();
    if (_size == _capacity)
      reserve(a, _capacity ? _capacity * 2 : 4);
    for (std::size_t i = _size; i > idx; --i) {
      if (i < _size) {
        _data[i] = std::move(_data[i - 1]);
      } else {
        std::construct_at(_data + i, std::move(_data[i - 1]));
      }
    }
    std::construct_at(_data + idx, v);
    ++_size;
    return iterator(_data + idx);
  }

  template <class Memory>
  constexpr iterator insert(Memory &a, iterator pos, T &&v) {
    std::size_t idx = pos - begin();
    if (_size == _capacity)
      reserve(a, _capacity ? _capacity * 2 : 4);
    for (std::size_t i = _size; i > idx; --i) {
      if (i < _size) {
        _data[i] = std::move(_data[i - 1]);
      } else {
        std::construct_at(_data + i, std::move(_data[i - 1]));
      }
    }
    std::construct_at(_data + idx, std::move(v));
    ++_size;
    return iterator(_data + idx);
  }

  template <class Memory> constexpr T *erase(Memory &a, T *pos) {
    std::size_t idx = pos - _data;
    std::destroy_at(_data + idx);
    for (std::size_t i = idx; i < _size - 1; ++i)
      std::construct_at(_data + i, std::move(_data[i + 1]));
    --_size;
    return _data + idx;
  }

  template <class Memory>
  constexpr iterator erase(Memory &a, T *first, T *last) {
    std::size_t first_idx = first - _data;
    std::size_t last_idx = last - _data;
    std::size_t num_to_erase = last_idx - first_idx;

    for (std::size_t i = first_idx; i < last_idx; ++i)
      std::destroy_at(_data + i);

    for (std::size_t i = first_idx; i < _size - num_to_erase; ++i)
      std::construct_at(_data + i, std::move(_data[i + num_to_erase]));

    _size -= num_to_erase;
    return iterator(_data + first_idx);
  }
};

} // namespace ycetl
