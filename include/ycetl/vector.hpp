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

template <class T, class Memory> class vector;
/* detect nested vector --------------------------------------------------- */
template <class> struct is_vector : std::false_type {};
template <class U, class A> struct is_vector<vector<U, A>> : std::true_type {};

/* iterator --------------------------------------------------------------- */
template <class T, class Memory, bool Const> class vector_iterator {
  using storage_unit = backend_type_of_t<T>;
  using raw = std::conditional_t<Const, const storage_unit *, storage_unit *>;
  raw _ptr = nullptr;
  Memory *_memory = nullptr;

public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::conditional_t<Const, const T, T>;
  using difference_type = std::ptrdiff_t;
  using reference = value_type;

  constexpr vector_iterator() = default;
  constexpr vector_iterator(raw p, Memory *a) : _ptr(p), _memory(a) {}

  template <bool C = Const, typename = std::enable_if_t<!C>>
  constexpr operator vector_iterator<T, Memory, true>() const {
    return {_ptr, _memory};
  }

  constexpr value_type operator*() const {
    if constexpr (is_vector<T>::value)
      return T(*_memory, const_cast<storage_unit &>(*_ptr));
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
typename Memory = typename container::container_traits<T>::default_memory>
// clang-format on
class vector : public container::container<vector, T, Memory> {
public:
public:
  using base_type = container::container<vector, T, Memory>;
  using typename base_type::backend_type;
  using typename base_type::relevant_of;
  using typename base_type::storage_unit;
  using memory_type = Memory;

  using value_type = T;
  using size_type = std::size_t;
  using iterator = vec_iter<T, Memory>;
  using const_iterator = vec_citer<T, Memory>;

private:
  owned_pointer<Memory> _memory_ptr;
  owned_pointer<backend_type> _backend;

public:
  // special constructor mainly used for nested containers
  // the inner container will get a  downgraded multitype memory
  template <typename OtherMemory>
  constexpr vector(backend_type &backend, OtherMemory &other_memory)
      : _memory_ptr(other_memory), _backend(&backend) {}

  constexpr Memory &memory() { return *_memory_ptr; }
  constexpr const Memory &memory() const { return *_memory_ptr; }

  template <class, class> friend class vector;

  /* constructors (unchanged bodies, but _backend calls stay correct) */
  constexpr vector() : _memory_ptr(), _backend() {}
  explicit constexpr vector(Memory &a) : _memory_ptr(&a), _backend() {}

  constexpr vector(size_type n, const T &v)
      : _memory_ptr(), _backend(memory(), n, v) {}
  constexpr vector(size_type n, const T &v, Memory &a)
      : _memory_ptr(&a), _backend(memory(), n, v) {}
  explicit constexpr vector(size_type n)
      : _memory_ptr(), _backend(memory(), n) {}
  constexpr vector(size_type n, Memory &a)
      : _memory_ptr(&a), _backend(memory(), n) {}

  constexpr vector(std::initializer_list<T> il)
      : _memory_ptr(), _backend(memory(), il) {}
  constexpr vector(std::initializer_list<T> il, Memory &a)
      : _memory_ptr(&a), _backend(memory(), il) {}

  template <class It, typename = std::enable_if_t<!std::is_integral_v<It>>>
  constexpr vector(It f, It l)
      : _memory_ptr(),
        _backend(memory(), f, static_cast<size_type>(std::distance(f, l))) {}
  template <class It, typename = std::enable_if_t<!std::is_integral_v<It>>,
            typename = void>
  constexpr vector(It f, It l, Memory &a)
      : _memory_ptr(&a),
        _backend(memory(), f, static_cast<size_type>(std::distance(f, l))) {}

  constexpr vector(const vector &o)
      : _memory_ptr(), _backend(memory(), *o._backend) {}
  constexpr vector(const vector &o, Memory &a)
      : _memory_ptr(&a), _backend(memory(), *o._backend) {}

  constexpr vector(vector &&o) noexcept
      : _memory_ptr(std::move(o._memory_ptr)), _backend(std::move(o._backend)) {
  }

  constexpr vector(vector &&o, Memory &a) : _memory_ptr(&a), _backend() {
    reserve(o.size());
    for (auto &e : *o._backend)
      _backend->push_back(memory(), std::move(e));
    o.clear();
  }

  constexpr ~vector() = default;

  /* capacity ----------------------------------------------------------- */
  constexpr bool empty() const noexcept { return size() == 0; }
  constexpr size_type size() const noexcept { return _backend->size(); }
  constexpr size_type capacity() const noexcept { return _backend->capacity(); }

  constexpr void reserve(size_type n) { _backend->reserve(memory(), n); }
  constexpr void resize(size_type n) { _backend->resize(memory(), n); }
  constexpr void clear() { _backend->clear(); }

  /* element access ----------------------------------------------------- */
  constexpr T operator[](size_type i) {
    if constexpr (is_vector<T>::value)
      return T((*_backend)[i], memory());
    else
      return (*_backend)[i];
  }

  constexpr const T operator[](size_type i) const {
    if constexpr (is_vector<T>::value)
      return T((*_backend)[i], memory());
    else
      return (*_backend)[i];
  }

  /* modifiers ---------------------------------------------------------- */
  constexpr void push_back(const T &v) {
    if constexpr (is_vector<T>::value)
      // the first memory() is for the vector itself, the second for the
      // back-end
      _backend->emplace_back(memory(), memory(), *v._backend); // copy back‑end
    else
      _backend->push_back(memory(), v);
  }
  constexpr void push_back(T &&v) {
    if constexpr (is_vector<T>::value)
      _backend->emplace_back(memory(), std::move(*v._backend)); // move back‑end
    else
      _backend->push_back(memory(), std::move(v));
  }

  /*
  template <class... Args> constexpr T &emplace_back(Args &&...args) {
    return *_backend->emplace_back(memory(), std::forward<Args>(args)...);
  }
  */
  template <class... Args> constexpr auto emplace_back(Args &&...args) {
    if constexpr (is_vector<T>::value) {
      auto &inner_ref =
          *_backend->emplace_back(memory(), std::forward<Args>(args)...);
      return T(inner_ref, memory());
    } else {
      return (*_backend->emplace_back(memory(), std::forward<Args>(args)...));
    }
  }

  constexpr iterator insert(iterator pos, const T &val) {
    size_type idx = pos - begin();
    if constexpr (is_vector<T>::value)
      _backend->insert(memory(), _backend->begin() + idx, *val._backend);
    else
      _backend->insert(memory(), _backend->begin() + idx, val);
    return iterator(_backend->begin() + idx, &memory());
  }

  constexpr void pop_back() { _backend->pop_back(); }

  /* iterators ---------------------------------------------------------- */
  constexpr iterator begin() noexcept { return {_backend->begin(), &memory()}; }
  constexpr iterator end() noexcept { return {_backend->end(), &memory()}; }
  constexpr const_iterator begin() const noexcept {
    return {_backend->begin(), &memory()};
  }
  constexpr const_iterator end() const noexcept {
    return {_backend->end(), &memory()};
  }
};

} // namespace ycetl
