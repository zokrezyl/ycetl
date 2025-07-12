/*──────────────────────────────────────────────────────────────────────────────
  ycetl/vector.hpp
──────────────────────────────────────────────────────────────────────────────*/
#pragma once

#include <compare>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>         // owned_pointer, default_allocator
#include <ycetl/relevant_types.hpp> // relevant_type, relevant_types_t …

namespace ycetl {

template <class T, class Alloc> class vector;
/*──────── detect ycetl::vector --------------------------------------------*/
template <class> struct is_vector : std::false_type {};
template <class U, class A> struct is_vector<vector<U, A>> : std::true_type {};

/*──────── iterator adapter -------------------------------------------------*/
template <class T, class Alloc, bool Const> class vector_iterator {
  using elem_backend =
      typename relevant_type<T>::type; // int, dynamic_array<…>, …
  using raw_ptr =
      std::conditional_t<Const, const elem_backend *, elem_backend *>;

  raw_ptr _ptr = nullptr;
  Alloc *_alloc = nullptr;

public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::conditional_t<Const, const T, T>;
  using difference_type = std::ptrdiff_t;
  using reference = value_type; // return by value

  constexpr vector_iterator() = default;
  constexpr vector_iterator(raw_ptr p, Alloc *a) : _ptr(p), _alloc(a) {}

  template <bool C = Const, typename = std::enable_if_t<!C>>
  constexpr operator vector_iterator<T, Alloc, true>() const {
    return {_ptr, _alloc};
  }

  constexpr value_type operator*() const {
    if constexpr (is_vector<T>::value)
      return T(*_alloc,
               const_cast<elem_backend &>(*_ptr)); // non‑owning wrapper
    else
      return *_ptr; // element copy
  }
  constexpr value_type operator[](difference_type n) const {
    return *(*this + n);
  }

  /* pointer arithmetic, comparisons */
  constexpr vector_iterator &operator++() {
    ++_ptr;
    return *this;
  }
  constexpr vector_iterator operator++(int) {
    auto t = *this;
    ++*this;
    return t;
  }
  constexpr vector_iterator &operator--() {
    --_ptr;
    return *this;
  }
  constexpr vector_iterator operator--(int) {
    auto t = *this;
    --*this;
    return t;
  }
  constexpr vector_iterator &operator+=(difference_type n) {
    _ptr += n;
    return *this;
  }
  constexpr vector_iterator &operator-=(difference_type n) {
    _ptr -= n;
    return *this;
  }
  friend constexpr vector_iterator operator+(vector_iterator it,
                                             difference_type n) {
    it += n;
    return it;
  }
  friend constexpr vector_iterator operator+(difference_type n,
                                             vector_iterator it) {
    it += n;
    return it;
  }
  friend constexpr vector_iterator operator-(vector_iterator it,
                                             difference_type n) {
    it -= n;
    return it;
  }
  friend constexpr difference_type operator-(vector_iterator l,
                                             vector_iterator r) {
    return l._ptr - r._ptr;
  }

  friend constexpr bool operator==(vector_iterator l, vector_iterator r) {
    return l._ptr == r._ptr;
  }
  friend constexpr auto operator<=>(vector_iterator l, vector_iterator r) {
    return l._ptr <=> r._ptr;
  }
};

template <class T, class A> using vec_iter = vector_iterator<T, A, false>;
template <class T, class A> using vec_citer = vector_iterator<T, A, true>;

/*────────────────────────────── vector  ───────────────────────────────────*/
template <class T, class Allocator = default_allocator<T>> class vector {
  /* publish the back‑end so relevant_type can recurse */
public:
  using relevant_of = dynamic_array<typename relevant_type<T>::type>;

private:
  using storage_type = relevant_of; // alias for clarity

  owned_pointer<Allocator> _alloc_ptr;  // own or alias
  owned_pointer<storage_type> _storage; // always own

  constexpr Allocator &alloc() { return *_alloc_ptr; }
  constexpr const Allocator &alloc() const { return *_alloc_ptr; }

  /* non‑owning view ctor (used by iterator / operator[]) */
  constexpr vector(Allocator &a, storage_type &existing)
      : _alloc_ptr(&a), _storage(&existing) {}

  template <class, class> friend class vector;

public:
  using value_type = T;
  using size_type = std::size_t;
  using iterator = vec_iter<T, Allocator>;
  using const_iterator = vec_citer<T, Allocator>;

  /*──── constructors (STL parity) ─────────────────────────────────*/
  constexpr vector() : _alloc_ptr(), _storage() {}
  explicit constexpr vector(Allocator &a) : _alloc_ptr(&a), _storage() {}

  constexpr vector(size_type n, const T &v)
      : _alloc_ptr(), _storage(alloc(), n, v) {}
  constexpr vector(size_type n, const T &v, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), n, v) {}

  explicit constexpr vector(size_type n) : _alloc_ptr(), _storage(alloc(), n) {}
  constexpr vector(size_type n, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), n) {}

  constexpr vector(std::initializer_list<T> il)
      : _alloc_ptr(), _storage(alloc(), il) {}
  constexpr vector(std::initializer_list<T> il, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), il) {}

  template <class It, typename = std::enable_if_t<!std::is_integral_v<It>>>
  constexpr vector(It first, It last)
      : _alloc_ptr(),
        _storage(alloc(), first,
                 static_cast<size_type>(std::distance(first, last))) {}

  template <class It, typename = std::enable_if_t<!std::is_integral_v<It>>,
            typename = void>
  constexpr vector(It first, It last, Allocator &a)
      : _alloc_ptr(&a),
        _storage(alloc(), first,
                 static_cast<size_type>(std::distance(first, last))) {}

  constexpr vector(const vector &other)
      : _alloc_ptr(), _storage(alloc(), *other._storage) {}
  constexpr vector(const vector &other, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), *other._storage) {}

  constexpr vector(vector &&other) noexcept
      : _alloc_ptr(std::move(other._alloc_ptr)),
        _storage(std::move(other._storage)) {}

  constexpr vector(vector &&other, Allocator &a) : _alloc_ptr(&a), _storage() {
    reserve(other.size());
    for (auto &e : *other._storage)
      _storage->push_back(std::move(e), alloc());
    other.clear();
  }

  constexpr ~vector() = default;

  /*──── capacity ─────────────────────────────────────────────────*/
  constexpr bool empty() const noexcept { return size() == 0; }
  constexpr size_type size() const noexcept { return _storage->size(); }
  constexpr size_type capacity() const noexcept { return _storage->capacity(); }

  constexpr void reserve(size_type n) { _storage->reserve(alloc(), n); }
  constexpr void resize(size_type n) { _storage->resize(alloc(), n); }
  constexpr void clear() { _storage->clear(); }

  /*──── element access ───────────────────────────────────────────*/
  constexpr T operator[](size_type i) {
    if constexpr (is_vector<T>::value)
      return T(alloc(), (*_storage)[i]);
    else
      return (*_storage)[i];
  }
  constexpr const T operator[](size_type i) const {
    if constexpr (is_vector<T>::value)
      return T(alloc(), (*_storage)[i]);
    else
      return (*_storage)[i];
  }

  /*──── modifiers ────────────────────────────────────────────────*/
  constexpr void push_back(const T &v) {
    if constexpr (is_vector<T>::value)
      _storage->push_back(*v._storage, alloc()); // copy backend
    else
      _storage->push_back(v, alloc());
  }
  constexpr void push_back(T &&v) {
    if constexpr (is_vector<T>::value)
      _storage->push_back(std::move(*v._storage), alloc()); // move backend
    else
      _storage->push_back(std::move(v), alloc());
  }

  template <class... Args> constexpr T &emplace_back(Args &&...args) {
    return *_storage->emplace_back(alloc(), std::forward<Args>(args)...);
  }

  constexpr iterator insert(iterator pos, const T &value) {
    std::size_t idx = pos - begin();
    auto *buf = _storage.get();
    if constexpr (is_vector<T>::value)
      buf->insert(alloc(), buf->begin() + idx, *value._storage);
    else
      buf->insert(alloc(), buf->begin() + idx, value);
    return iterator(buf->begin() + idx, &alloc());
  }

  constexpr void pop_back() { _storage->pop_back(); }

  /*──── iterators ────────────────────────────────────────────────*/
  constexpr iterator begin() noexcept {
    return iterator(_storage->begin(), &alloc());
  }
  constexpr iterator end() noexcept {
    return iterator(_storage->end(), &alloc());
  }
  constexpr const_iterator begin() const noexcept {
    return const_iterator(_storage->begin(), &alloc());
  }
  constexpr const_iterator end() const noexcept {
    return const_iterator(_storage->end(), &alloc());
  }
};

} // namespace ycetl
