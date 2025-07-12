#pragma once
#include <initializer_list>
#include <iterator>
#include <utility>
#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

template <class T, class Alloc> class vector;

/* allocation_type_of for nested vectors */
template <class T> struct allocation_type_of {
  using type = T;
};

template <class T, class A> struct allocation_type_of<vector<T, A>> {
  using type = dynamic_array<typename allocation_type_of<T>::type>;
};

template <class T>
using allocation_type_of_t = typename allocation_type_of<T>::type;

/* --------------------------------------------------------------------- */
template <class T, class Allocator> class vector {
  using storage_type = allocation_type_of_t<vector<T, Allocator>>;

  Allocator _alloc;
  ycetl::owned_pointer<storage_type> _storage;

public:
  using value_type = T;
  using size_type = std::size_t;
  using reference = T &;
  using const_reference = const T &;
  using iterator = value_type *;
  using const_iterator = const value_type *;

  /* constructors ------------------------------------------------------ */
  constexpr explicit vector(Allocator a = Allocator{})
      : _alloc(a), _storage() {}

  constexpr vector(size_type n, const T &v, Allocator a = Allocator{})
      : _alloc(a), _storage(_alloc, n, v) {}

  constexpr explicit vector(size_type n, Allocator a = Allocator{})
      : _alloc(a), _storage(_alloc, n) {}

  constexpr vector(std::initializer_list<T> il, Allocator a = Allocator{})
      : _alloc(a), _storage(_alloc, il.begin(), il.size()) {}

  constexpr vector(const vector &other)
      : _alloc(other._alloc), _storage(_alloc, other.begin(), other.size()) {}

  constexpr vector(vector &&other) noexcept
      : _alloc(std::move(other._alloc)), _storage(std::move(other._storage)) {}

  constexpr vector &operator=(const vector &other) {
    if (this != &other) {
      clear();
      reserve(other.size());
      for (const auto &x : other)
        push_back(x);
    }
    return *this;
  }

  constexpr vector &operator=(vector &&other) noexcept {
    _storage = std::move(other._storage);
    _alloc = std::move(other._alloc);
    other._storage = {};
    return *this;
  }

  constexpr ~vector() = default;

  /* capacity ---------------------------------------------------------- */
  constexpr bool empty() const noexcept { return size() == 0; }
  constexpr size_type size() const noexcept { return _storage.get()->size(); }
  constexpr size_type capacity() const noexcept {
    return _storage.get()->capacity();
  }

  constexpr void reserve(size_type c) { _storage.get()->reserve(_alloc, c); }
  constexpr void resize(size_type n) { _storage.get()->resize(_alloc, n); }
  constexpr void clear() { _storage.get()->clear(); }

  /* element access ---------------------------------------------------- */
  constexpr reference operator[](size_type i) { return _storage[i]; }
  constexpr const_reference operator[](size_type i) const {
    return _storage[i];
  }

  constexpr reference front() { return _storage[0]; }
  constexpr const_reference front() const { return _storage[0]; }

  constexpr reference back() { return _storage[size() - 1]; }
  constexpr const_reference back() const { return _storage[size() - 1]; }

  /* modifiers --------------------------------------------------------- */
  constexpr void push_back(const T &v) { _storage.get()->push_back(v, _alloc); }
  constexpr void push_back(T &&v) {
    _storage.get()->push_back(std::move(v), _alloc);
  }

  template <class... Args> constexpr reference emplace_back(Args &&...args) {
    return *_storage.get()->emplace_back(_alloc, std::forward<Args>(args)...);
  }

  constexpr void pop_back() { _storage.get()->pop_back(); }

  /* iterators --------------------------------------------------------- */
  constexpr iterator begin() noexcept { return _storage.get()->begin(); }
  constexpr iterator end() noexcept { return _storage.get()->end(); }
  constexpr const_iterator begin() const noexcept {
    return _storage.get()->begin();
  }
  constexpr const_iterator end() const noexcept {
    return _storage.get()->end();
  }
};

} // namespace ycetl
