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

template <typename> struct is_vector : std::false_type {};
template <typename U, typename A>
struct is_vector<vector<U, A>> : std::true_type {};

// ── iterator template
// ------------------------------------------------------------
template <class T, class Allocator, bool Const> class vector_iterator {
  using storage_elem = allocation_type_of_t<T>;
  using raw_ptr =
      std::conditional_t<Const, const storage_elem *, storage_elem *>;
  raw_ptr _ptr = nullptr;      // points into dynamic_array buffer
  Allocator *_alloc = nullptr; // points to outer allocator

public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::conditional_t<Const, const T, T>;
  using difference_type = std::ptrdiff_t;
  using pointer = void;         // not provided
  using reference = value_type; // we return by value

  // constructors --------------------------------------------------------
  constexpr vector_iterator() = default;
  constexpr vector_iterator(raw_ptr p, Allocator *a) : _ptr(p), _alloc(a) {}

  // implicit conversion  iterator -> const_iterator
  template <bool C = Const, typename = std::enable_if_t<!C>>
  constexpr operator vector_iterator<T, Allocator, true>() const {
    return {_ptr, _alloc};
  }

  // dereference ---------------------------------------------------------
  constexpr value_type operator*() const {
    if constexpr (is_vector<T>::value) {
      return T(*_alloc, const_cast<storage_elem &>(*_ptr)); // wrapper
    } else {
      return *_ptr; // element copy
    }
  }
  constexpr value_type operator[](difference_type n) const {
    return *(*this + n);
  }

  // pointer arithmetic --------------------------------------------------
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

  // comparisons ---------------------------------------------------------
  friend constexpr bool operator==(vector_iterator l, vector_iterator r) {
    return l._ptr == r._ptr;
  }
  friend constexpr auto operator<=>(vector_iterator l, vector_iterator r) {
    return l._ptr <=> r._ptr;
  }
};

// ── convenient aliases ---------------------------------------------------
template <class T, class Alloc>
using vec_iter = vector_iterator<T, Alloc, false>;
template <class T, class Alloc>
using vec_citer = vector_iterator<T, Alloc, true>;

/* --------------------------------------------------------------------- */
// TODO: deduce Allocator = ycetl::default_allocator<defuce<alloccator<T>>
template <class T, class Allocator> class vector {
  using storage_type = allocation_type_of_t<vector<T, Allocator>>;

private:
  Allocator _alloc;
  ycetl::owned_pointer<storage_type> _storage;

private:
  // this special constructor is used to create a vector used with operator []
#if 0
  constexpr vector(typename allocation_type_of_t<vector>::type &inner,
                   Allocator &alloc)
      : _alloc(alloc), _storage(&inner) {}
#endif

public:
  using value_type = T;
  using size_type = std::size_t;
  using reference = T &;
  using const_reference = const T &;
  using iterator = vec_iter<T, Allocator>;
  using const_iterator = vec_citer<T, Allocator>;

  /* ---------------------------------------------------------------------- */
  /* 1. default                                                             */
  constexpr vector() // (1)
      : _alloc(), _storage() {}

  explicit constexpr vector(const Allocator &alloc) // (2)
      : _alloc(alloc), _storage() {}

  /* ---------------------------------------------------------------------- */
  /* 2. size + value                                                        */
  constexpr vector(size_type n, const T &value) // (3)
      : _alloc(), _storage(_alloc, n, value) {}

  constexpr vector(size_type n, const T &value,
                   const Allocator &alloc) // (4)
      : _alloc(alloc), _storage(_alloc, n, value) {}

  /* ---------------------------------------------------------------------- */
  /* 3. size only (value‑initialised)                                       */
  explicit constexpr vector(size_type n) // (5)
      : _alloc(), _storage(_alloc, n) {}

  constexpr vector(size_type n, const Allocator &alloc) // (6)
      : _alloc(alloc), _storage(_alloc, n) {}

  /* ---------------------------------------------------------------------- */
  /* 4. range [first,last)                                                  */
  template <class InputIt,
            typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
  constexpr vector(InputIt first, InputIt last) // (7)
      : _alloc(), _storage(_alloc, first,
                           static_cast<size_type>(std::distance(first, last))) {
  }

  template <class InputIt,
            typename = std::enable_if_t<!std::is_integral_v<InputIt>>,
            typename = void>
  constexpr vector(InputIt first, InputIt last,
                   const Allocator &alloc) // (8)
      : _alloc(alloc),
        _storage(_alloc, first,
                 static_cast<size_type>(std::distance(first, last))) {}

  /* ---------------------------------------------------------------------- */
  /* 5. initializer‑list                                                    */
  constexpr vector(std::initializer_list<T> il) // (9)
      : _alloc(), _storage(_alloc, il) {}

  constexpr vector(std::initializer_list<T> il,
                   const Allocator &alloc) // (10)
      : _alloc(alloc), _storage(_alloc, il) {}

  /* ---------------------------------------------------------------------- */
  /* 6. copy                                                                */
  constexpr vector(const vector &other) // (11)
      : _alloc(other._alloc), _storage(_alloc, *other._storage.get()) {}

  constexpr vector(const vector &other,
                   const Allocator &alloc) // (12)
      : _alloc(alloc), _storage(_alloc, *other._storage.get()) {}

  /* ---------------------------------------------------------------------- */
  /* 7. move                                                                */
  constexpr vector(vector &&other) noexcept // (13)
      : _alloc(std::move(other._alloc)), _storage(std::move(other._storage)) {}

  constexpr vector(vector &&other,
                   const Allocator &alloc) // (14)
      : _alloc(alloc), _storage(std::move(other._storage)) {}

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

  // ─── non‑const ───────────────────────────────────────────────────────────
  constexpr T operator[](size_type i) {
    if constexpr (is_vector<T>::value) {
      auto &inner = _storage.get()->operator[](i); // dynamic_array<...>&
      return T(_alloc, inner);                     // non‑owning wrapper
    } else {
      return _storage.get()->operator[](i); // copy of element
    }
  }

  // ─── const ‑ access (read‑only) ──────────────────────────────────────────
  constexpr const T operator[](size_type i) const {
    if constexpr (is_vector<T>::value) {
      auto &inner = _storage.get()->operator[](i);
      return T(_alloc, inner); // wrapper (read‑only use)
    } else {
      return _storage.get()->operator[](i); // copy, but const
    }
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

  constexpr iterator begin() noexcept {
    return iterator(_storage->data(), _alloc);
  }
  constexpr iterator end() noexcept {
    return iterator(_storage->data() + size(), _alloc);
  }
  constexpr const_iterator begin() const noexcept {
    return const_iterator(_storage->data(), _alloc);
  }
  constexpr const_iterator end() const noexcept {
    return const_iterator(_storage->data() + size(), _alloc);
  }
};

} // namespace ycetl
