#pragma once

#include <compare>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

#include <ycetl/impl/container.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

template <class T, class Alloc> class vector;
/* detect nested vector --------------------------------------------------- */
template <class> struct is_vector : std::false_type {};
template <class U, class A> struct is_vector<vector<U, A>> : std::true_type {};

/* iterator --------------------------------------------------------------- */
template <class T, class Alloc, bool Const> class vector_iterator {
  using storage_unit = storage_type_of_t<T>;
  using raw = std::conditional_t<Const, const storage_unit *, storage_unit *>;
  raw _ptr = nullptr;
  Alloc *_alloc = nullptr;

public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::conditional_t<Const, const T, T>;
  using difference_type = std::ptrdiff_t;
  using reference = value_type;

  constexpr vector_iterator() = default;
  constexpr vector_iterator(raw p, Alloc *a) : _ptr(p), _alloc(a) {}

  template <bool C = Const, typename = std::enable_if_t<!C>>
  constexpr operator vector_iterator<T, Alloc, true>() const {
    return {_ptr, _alloc};
  }

  constexpr value_type operator*() const {
    if constexpr (is_vector<T>::value)
      return T(*_alloc, const_cast<storage_unit &>(*_ptr));
    else
      return *_ptr;
  }
  constexpr value_type operator[](difference_type n) const {
    return *(*this + n);
  }

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

/*──────────────────────────── vector ─────────────────────────────────────*/
// clang-format off
template <typename T, 
typename Allocator = typename container::container<T>::default_allocator>
// clang-format on
class vector : public container::container<T, Allocator> {
public:
public:
  using base_type = container::container<T, Allocator>;
  using typename base_type::relevant_of;
  using typename base_type::storage_type;
  using typename base_type::storage_unit;
  using allocator_type = Allocator;

  using value_type = T;
  using size_type = std::size_t;
  using iterator = vec_iter<T, Allocator>;
  using const_iterator = vec_citer<T, Allocator>;

private:
  owned_pointer<Allocator> _alloc_ptr;
  owned_pointer<storage_type> _storage;
  constexpr vector(storage_type &storage, Allocator &alloc)
      : _alloc_ptr(&alloc), _storage(storage) {}

public:
  constexpr Allocator &alloc() { return *_alloc_ptr; }
  constexpr const Allocator &alloc() const { return *_alloc_ptr; }

  constexpr vector(Allocator &a, storage_type &s)
      : _alloc_ptr(&a), _storage(&s) {}
  template <class, class> friend class vector;

  /* constructors (unchanged bodies, but _storage calls stay correct) */
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
  constexpr vector(It f, It l)
      : _alloc_ptr(),
        _storage(alloc(), f, static_cast<size_type>(std::distance(f, l))) {}
  template <class It, typename = std::enable_if_t<!std::is_integral_v<It>>,
            typename = void>
  constexpr vector(It f, It l, Allocator &a)
      : _alloc_ptr(&a),
        _storage(alloc(), f, static_cast<size_type>(std::distance(f, l))) {}

  constexpr vector(const vector &o)
      : _alloc_ptr(), _storage(alloc(), *o._storage) {}
  constexpr vector(const vector &o, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), *o._storage) {}

  constexpr vector(vector &&o) noexcept
      : _alloc_ptr(std::move(o._alloc_ptr)), _storage(std::move(o._storage)) {}

  constexpr vector(vector &&o, Allocator &a) : _alloc_ptr(&a), _storage() {
    reserve(o.size());
    for (auto &e : *o._storage)
      _storage->push_back(alloc(), std::move(e));
    o.clear();
  }

  constexpr ~vector() = default;

  /* capacity ----------------------------------------------------------- */
  constexpr bool empty() const noexcept { return size() == 0; }
  constexpr size_type size() const noexcept { return _storage->size(); }
  constexpr size_type capacity() const noexcept { return _storage->capacity(); }

  constexpr void reserve(size_type n) { _storage->reserve(alloc(), n); }
  constexpr void resize(size_type n) { _storage->resize(alloc(), n); }
  constexpr void clear() { _storage->clear(); }

  /* element access ----------------------------------------------------- */
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

  /* modifiers ---------------------------------------------------------- */
  constexpr void push_back(const T &v) {
    if constexpr (is_vector<T>::value)
      // the first alloc() is for the vector itself, the second for the back-end
      _storage->emplace_back(alloc(), alloc(), *v._storage); // copy back‑end
    else
      _storage->push_back(alloc(), v);
  }
  constexpr void push_back(T &&v) {
    if constexpr (is_vector<T>::value)
      _storage->emplace_back(alloc(), std::move(*v._storage)); // move back‑end
    else
      _storage->push_back(alloc(), std::move(v));
  }

  /*
  template <class... Args> constexpr T &emplace_back(Args &&...args) {
    return *_storage->emplace_back(alloc(), std::forward<Args>(args)...);
  }
  */
  template <class... Args> constexpr auto emplace_back(Args &&...args) {
    if constexpr (is_vector<T>::value) {
      return T(*_storage->emplace_back(std::forward<Args>(args)...), alloc());
    } else {
      return (*_storage->emplace_back(alloc(), std::forward<Args>(args)...));
    }
  }

  constexpr iterator insert(iterator pos, const T &val) {
    size_type idx = pos - begin();
    if constexpr (is_vector<T>::value)
      _storage->insert(alloc(), _storage->begin() + idx, *val._storage);
    else
      _storage->insert(alloc(), _storage->begin() + idx, val);
    return iterator(_storage->begin() + idx, &alloc());
  }

  constexpr void pop_back() { _storage->pop_back(); }

  /* iterators ---------------------------------------------------------- */
  constexpr iterator begin() noexcept { return {_storage->begin(), &alloc()}; }
  constexpr iterator end() noexcept { return {_storage->end(), &alloc()}; }
  constexpr const_iterator begin() const noexcept {
    return {_storage->begin(), &alloc()};
  }
  constexpr const_iterator end() const noexcept {
    return {_storage->end(), &alloc()};
  }
};

} // namespace ycetl
