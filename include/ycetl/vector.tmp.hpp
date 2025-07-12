#pragma once
#include <initializer_list>
#include <iterator>
#include <ycetl/dynamic_array.hpp> // assuming this is the previous implementation
#include <ycetl/memory.hpp> // assuming this is the previous implementation

namespace ycetl {

template <typename T, typename Allocator> class vector;

template <typename T> struct allocation_type_of {
  using type = T;
};

template <typename T, typename Alloc>
struct allocation_type_of<vector<T, Alloc>> {
  using type = dynamic_array<typename allocation_type_of<T>::type>;
};

template <typename T>
using allocation_type_of_t = typename allocation_type_of<T>::type;

template <typename T, typename Allocator = default_allocator<T>> class vector {
public:
  using value_type = T;
  using allocator_type = Allocator;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  using alloc_type = allocation_type_of_t<vector<T, Allocator>>;

  Allocator _alloc;
  ycetl::owned_pointer<alloc_type> _storage;

public:
  // Constructors
  constexpr vector() : _alloc(Allocator{}), _storage() {}

  constexpr vector(Allocator &alloc) : _storage(), _alloc(alloc) {}

  constexpr vector(size_type count, const T &value)
      : _alloc(Allocator{}), _storage(_alloc, count, value) {}

  constexpr vector(size_type count, const T &value, Allocator &alloc)
      : _alloc(alloc), _storage(alloc, count, value) {}

  constexpr explicit vector(size_type count, Allocator &alloc)
      : _alloc(alloc), _storage(alloc, count) {}

  constexpr vector(std::initializer_list<T> ilist,
                   Allocator alloc = Allocator{})
      : _storage(ilist.begin(), ilist.size(), alloc), _alloc(alloc) {}

  constexpr vector(const vector &other)
      : _alloc(other._alloc), _storage(_alloc, other._storage.get()->data()) {}

  constexpr vector(vector &&other) noexcept
      : _alloc(std::move(other._alloc), _storage(_alloc, std::move(other._storage)) {}

  constexpr vector &operator=(const vector &other) {
    if (this != &other) {
      clear();
      reserve(other.size());
      for (size_type i = 0; i < other.size(); ++i)
        push_back(other[i]);
    }
    return *this;
  }

  constexpr vector &operator=(vector &&other) noexcept {
    _storage = other._storage;
    _alloc = std::move(other._alloc);
    other._storage = {};
    return *this;
  }

  constexpr ~vector() = default;

  // Capacity
  constexpr bool empty() const noexcept {
    return _storage.get()->size() == 0; }
  constexpr size_type size() const noexcept {
    return _storage.get()->size(); }
  constexpr size_type capacity() const noexcept {
    return _storage.get()->capacity();
  }

  constexpr void reserve(size_type new_cap) {
    _storage.get()->reserve(new_cap, _alloc);
  }

  constexpr void resize(size_type new_size) {
    _storage.get()->resize(new_size, _alloc);
  }

  constexpr void clear() noexcept {
    _storage.get()->clear(); }

  constexpr iterator insert(iterator pos, const T &value) {
    return _storage.get()->insert(_alloc, pos, value);
  }

  template <typename... Args> constexpr reference emplace_back(Args &&...args) {
    return *_storage.get()->emplace_back(_alloc, std::forward<Args>(args)...);
  }

  // Element access
  constexpr reference operator[](size_type pos) {
    return _storage.get()->operator[](pos);
  }

  constexpr const_reference operator[](size_type pos) const {
    return _storage.get()->operator[](pos);
  }

  constexpr reference front() {
    return _storage.get()->operator[](0); }
  constexpr const_reference front() const {
    return _storage.get()->operator[](0);
  }
  constexpr reference back() {
    return _storage[_storage.get()->size() - 1]; }
  constexpr const_reference back() const {
    return _storage.get()->operator[](_storage.size() - 1);
  }

  /*
  constexpr T *data() noexcept { return _storage.data(); }
  constexpr const T *data() const noexcept { return _storage.data(); }
*/

  // Modifiers
  constexpr void push_back(const T &value) {
    _storage.get()->push_back(value, _alloc);
  }

  constexpr void push_back(T &&value) {
    _storage.get()->push_back(std::move(value), _alloc);
  }

  constexpr void pop_back() {
    _storage.get()->pop_back(); }

  constexpr void swap(vector &other) noexcept {
    std::swap(_storage, other._storage);
    std::swap(_alloc, other._alloc);
  }

  // Iterators
  constexpr iterator begin() noexcept {
    return _storage.get()->begin(); }
  constexpr iterator end() noexcept {
    return _storage.get()->end(); }
  constexpr const_iterator begin() const noexcept {
    return _storage.get()->begin();
  }
  constexpr const_iterator end() const noexcept {
    return _storage.get()->end();
  }
  constexpr const_iterator cbegin() const noexcept {
    return _storage.get()->begin();
  }
  constexpr const_iterator cend() const noexcept {
    return _storage.get()->end();
  }

  constexpr reverse_iterator rbegin() noexcept {
    return reverse_iterator(end());
  }
  constexpr reverse_iterator rend() noexcept {
    return reverse_iterator(begin());
  }
  constexpr const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  constexpr const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }
  constexpr const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  constexpr const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }

  // Allocator
  constexpr allocator_type get_allocator() const noexcept {
    return _alloc; }
};
} // namespace ycetl
