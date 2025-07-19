#pragma once
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>

#include <ycetl/memory.hpp>

namespace ycetl {

template <typename T> auto none() noexcept { return T{}; }

template <typename T, typename TypedMemory = typed_dynamic_memory<T>>
class dynamic_array {

public:
  using typed_memory = TypedMemory;
  using pointer = typename typed_memory::pointer;
  using size_type = typename typed_memory::size_type;
  using difference_type = typename typed_memory::difference_type;

private:
  pointer _data;
  size_type _size = 0;
  size_type _capacity = 0;

#if 0
  template <class... Args>
  constexpr void construct(size_type i, Args &&...args) {
    std::construct_at(_data + i, std::forward<Args>(args)...);
  }

#endif
  constexpr void destroy_range(size_type first, size_type last) noexcept {
    for (size_type i = first; i < last; ++i)
      destroy_at(_data + i);
  }

public:
  using value_type = T;
  using reference = T &;
  using const_reference = const T &;
  using const_pointer = const pointer;

  template <bool IsConst> class basic_iterator {

    friend class dynamic_array;

  public:
    using typed_memory = TypedMemory;
    using pointer = typename typed_memory::pointer;
    using const_pointer = typename typed_memory::const_pointer;
    using size_type = typename typed_memory::size_type;
    using difference_type = typename typed_memory::difference_type;

  private:
    pointer _ptr;

    using pointer_type = std::conditional_t<IsConst, const_pointer, pointer>;
    using reference_type =
        std::conditional_t<IsConst, const_reference, reference>;

  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using reference = reference_type;

    friend constexpr basic_iterator operator+(difference_type n,
                                              basic_iterator it) noexcept {
      return it + n;
    }

    constexpr basic_iterator() = delete;

    constexpr basic_iterator(pointer_type ptr) noexcept : _ptr(ptr) {}

    constexpr operator basic_iterator<true>() const noexcept
      requires(!IsConst)
    {
      return basic_iterator<true>(_ptr);
    }

    constexpr const_reference operator*() const noexcept { return *_ptr; }
    constexpr reference operator*() noexcept { return *_ptr; }

    constexpr pointer operator->() const noexcept { return *_ptr; }

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

  // we provide three categories of constructors:
  // 1. without memory, using default typed_memory
  // 2. with typed_memory, which is a memory handler for a specific type
  // 3. with multitype_memory, which can handle multiple types

  // allocators without input memory
  constexpr dynamic_array() noexcept : _data(typed_memory().none()) {}

  constexpr dynamic_array(size_type n) noexcept
      : _data(typed_memory().allocate(n)), _size(n), _capacity(n) {
    construct_n(_data, n);
  }

  constexpr dynamic_array(size_type n, const T &v) noexcept
      : _data(typed_memory().allocate(n)), _size(n), _capacity(n) {
    construct_n(_data, n, v);
  }

  constexpr dynamic_array(const dynamic_array &other)
      : _data(other._data.memory().allocate(other._capacity)),
        _size(other._size), _capacity(other._capacity) {
    copy_construct_n(other._data[0], _size, _data[0]);
  };

  // allocators with typed_memory

  constexpr dynamic_array(typed_memory &memory) noexcept
      : _data(memory.none()) {}

  constexpr dynamic_array(typed_memory &memory, size_type n) noexcept
      : _data(memory.allocate(n)), _size(n), _capacity(n) {
    construct_n(_data, n);
  }

  constexpr dynamic_array(typed_memory &memory, size_type n,
                          const_reference v) noexcept
      : _data(memory.allocate(n)), _size(n), _capacity(n) {
    construct_n(_data, n, v);
  }

  constexpr dynamic_array(typed_memory &mem, const dynamic_array &other)
      : _data(mem.allocate(other._capacity)), _size(other._size),
        _capacity(other._capacity) {
    copy_construct_n(other._data[0], _size, _data[0]);
  };

  // allocators with multitype_memory
  template <class MultitypeMemory>
  constexpr dynamic_array(MultitypeMemory &memory, size_type n)
      : _data(::ycetl::allocate<T>(memory, n)), _size(n), _capacity(n) {
    construct_n(_data, n);
  }

  template <class MultitypeMemory>
  constexpr dynamic_array(MultitypeMemory &memory, size_type n,
                          const_reference v)
      : _data(::ycetl::allocate<T>(memory, n)), _size(n), _capacity(n) {
    construct_n(_data, n);
  }

#if 0
  template <class Memory, class It>
  constexpr dynamic_array(Memory &a, It first, size_type n)
    requires std::is_base_of_v<
                 std::input_iterator_tag,
                 typename std::iterator_traits<It>::iterator_category>
      : _data(ycetl::allocate<value_type>(a, n)), _size(n), _capacity(n) {
    for (size_type i = 0; i < n; ++i)
      construct(i, *(first + i));
  }
#endif

  template <class Memory>
  constexpr dynamic_array(Memory &a, std::initializer_list<T> il)
      : dynamic_array(a, il.begin(), il.size()) {}

#if 0
  constexpr dynamic_array(dynamic_array &&other) noexcept
      : _data(std::exchange(other._data, nullptr)),
        _size(std::exchange(other._size, 0)),
        _capacity(std::exchange(other._capacity, 0)) {}
#endif

  dynamic_array &operator=(const dynamic_array &) = delete;

  constexpr iterator begin() noexcept { return iterator(_data); }
  constexpr iterator end() noexcept { return iterator(_data + _size); }

  constexpr pointer data() noexcept { return _data; }

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

  constexpr dynamic_array &operator=(dynamic_array &&other) noexcept {
    if (this != &other) {
      clear();
      _data = std::exchange(other._data, nullptr);
      _size = std::exchange(other._size, 0);
      _capacity = std::exchange(other._capacity, 0);
    }
    return *this;
  }

  constexpr ~dynamic_array() { clear(); }

  constexpr size_type size() const { return _size; }
  constexpr size_type capacity() const { return _capacity; }

  constexpr T &operator[](size_type i) { return _data[i]; }
  constexpr const T &operator[](size_type i) const { return _data[i]; }

  constexpr void reserve(size_type new_cap) {
    if (new_cap <= _capacity)
      return;
    pointer new_buf = _data.memory().allocate(new_cap);
    for (size_type i = 0; i < _size; ++i)
      std::construct_at(new_buf + i, std::move(_data[i]));
    destroy_range(0, _size);
#if 0
    if (_data) {
      _data.memory().deallocate(_data, _capacity);
    }
#endif
    _data = new_buf;
    _capacity = new_cap;
  }

  constexpr void resize(size_type new_size) {
    if (new_size < _size) {
      destroy_range(new_size, _size);
    } else if (new_size > _size) {
      if (new_size > _capacity) {
        reserve(new_size);
      }
      for (size_type i = _size; i < new_size; ++i) {
        construct(i);
      }
    }
    _size = new_size;
  }

  constexpr void resize(size_type new_size, const T &v) {
    if (new_size < _size) {
      destroy_range(new_size, _size);
    } else if (new_size > _size) {
      if (new_size > _capacity) {
        reserve(new_size);
      }
      for (size_type i = _size; i < new_size; ++i) {
        construct(i, v);
      }
    }
    _size = new_size;
  }

  template <class... Args> constexpr reference emplace_back(Args &&...args) {
    if (_size == _capacity)
      reserve(_capacity ? _capacity * 2 : 4);
    construct(_size, std::forward<Args>(args)...);
    return _data[_size++];
  }

  constexpr void push_back(const T &v) { emplace_back(v); }

  constexpr void push_back(T &&v) { emplace_back(std::move(v)); }

  constexpr void pop_back() {
    --_size;
    std::destroy_at(_data + _size);
  }

  constexpr void clear() noexcept {
    destroy_range(0, _size);
    _size = 0;
  }

  constexpr void clear_and_deallocate_buffer() noexcept {
#if 0
    clear();
    if (_data) {
      ycetl::deallocate<T>(mem, _data, _capacity);
      _data = nullptr;
      _capacity = 0;
    }
#endif
  }

  constexpr iterator insert(iterator pos, const T &v) {
    size_type idx = pos - begin();
    if (_size == _capacity)
      reserve(_capacity ? _capacity * 2 : 4);
    for (size_type i = _size; i > idx; --i) {
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

  constexpr iterator insert(iterator pos, T &&v) {
    size_type idx = pos - begin();
    if (_size == _capacity)
      reserve(_capacity ? _capacity * 2 : 4);
    for (size_type i = _size; i > idx; --i) {
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

  constexpr pointer erase(pointer pos) {
    size_type idx = pos - _data;
    std::destroy_at(_data + idx);
    for (size_type i = idx; i < _size - 1; ++i)
      std::construct_at(_data + i, std::move(_data[i + 1]));
    --_size;
    return _data + idx;
  }

  constexpr iterator erase(pointer first, pointer last) {
    size_type first_idx = first - _data;
    size_type last_idx = last - _data;
    size_type num_to_erase = last_idx - first_idx;

    for (size_type i = first_idx; i < last_idx; ++i)
      std::destroy_at(_data + i);

    for (size_type i = first_idx; i < _size - num_to_erase; ++i)
      std::construct_at(_data + i, std::move(_data[i + num_to_erase]));

    _size -= num_to_erase;
    return iterator(_data + first_idx);
  }
};

} // namespace ycetl
