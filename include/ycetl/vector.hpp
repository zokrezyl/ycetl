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

template <typename T, typename Memory, typename BackendMode> class vector;
;

template <typename T,
          typename Memory =
              typename container::container_traits<vector, T>::default_memory,
          typename BackendMode = container::by_value>
class vector;

/* detect nested vector --------------------------------------------------- */
template <class> struct is_vector : std::false_type {};
template <class U, class A> struct is_vector<vector<U, A>> : std::true_type {};

template <typename Traits, typename T, typename Memory, bool Const>
class vector_iterator {
public:
  using view_type = typename Traits::view_type;

  using iterator_category = std::random_access_iterator_tag;
  using storage_unit = backend_type_of_t<T>;
  using raw = std::conditional_t<Const, const storage_unit *, storage_unit *>;

  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type *;
  using reference = value_type;

private:
  raw _ptr = nullptr;
  Memory _memory;

public:
  constexpr vector_iterator() = default;
  constexpr vector_iterator(raw p, Memory mem) : _ptr(p), _memory(mem) {}

  constexpr reference operator*() const {
    if constexpr (is_vector<T>::value)
      return view_type(_memory, const_cast<storage_unit &>(*_ptr));
    else
      return *_ptr;
  }

  // Minimal operations for random-access iterator:
  constexpr vector_iterator &operator++() {
    ++_ptr;
    return *this;
  }
  constexpr vector_iterator operator++(int) {
    auto tmp = *this;
    ++_ptr;
    return tmp;
  }
  constexpr vector_iterator &operator--() {
    --_ptr;
    return *this;
  }
  constexpr vector_iterator operator--(int) {
    auto tmp = *this;
    --_ptr;
    return tmp;
  }
  constexpr vector_iterator &operator+=(difference_type n) {
    _ptr += n;
    return *this;
  }
  constexpr vector_iterator &operator-=(difference_type n) {
    _ptr -= n;
    return *this;
  }

  constexpr reference operator[](difference_type n) const {
    return *(*this + n);
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

/*──────────────────────────── vector ─────────────────────────────────────*/
// clang-format off
template <typename T, 
  typename Memory,
  typename BackendMode>
class vector {
  // clang-format on
public:
  using traits = container::container_traits<vector, T, Memory>;

  using backend_type = typename traits::backend_type;
  using storage_unit = typename traits::storage_unit;
  using relevant_of = typename traits::relevant_of;
  using memory_type = typename traits::memory_type;
  using value_type = typename traits::value_type;
  using view_type = typename traits::view_type;

  using reference = typename traits::reference;
  using const_reference = typename traits::const_reference;
  using pointer = typename traits::pointer;
  using const_pointer = typename traits::const_pointer;
  using size_type = typename traits::size_type;
  using difference_type = typename traits::difference_type;

  using iterator = vector_iterator<traits, value_type, memory_type, false>;
  using const_iterator = vector_iterator<traits, value_type, memory_type, true>;

private:
  memory_type _memory;
  backend_type _backend;

public:
  // special constructor mainly used for nested containers
  // the inner container will get a  downgraded multitype memory
  template <typename OtherMemory>
  constexpr vector(backend_type backend, OtherMemory other_memory)
      : _memory(other_memory), _backend(backend) {}

  template <class, class, class> friend class vector;

  /* constructors (unchanged bodies, but _backend calls stay correct) */
  constexpr vector() : _memory(), _backend() {}
  explicit constexpr vector(memory_type mem) : _memory(mem), _backend() {}

  constexpr vector(size_type n, const T &v)
      : _memory(), _backend(_memory, n, v) {}
  constexpr vector(size_type n, const T &v, memory_type mem)
      : _memory(mem), _backend(_memory, n, v) {}
  explicit constexpr vector(size_type n) : _memory(), _backend(_memory, n) {}
  constexpr vector(size_type n, memory_type mem)
      : _memory(mem), _backend(_memory, n) {}

  constexpr vector(std::initializer_list<T> il)
      : _memory(), _backend(_memory, il) {}
  constexpr vector(std::initializer_list<T> il, memory_type mem)
      : _memory(mem), _backend(_memory, il) {}

  template <class It, typename = std::enable_if_t<!std::is_integral_v<It>>>
  constexpr vector(It f, It l)
      : _memory(),
        _backend(_memory, f, static_cast<size_type>(std::distance(f, l))) {}
  template <class It, typename = std::enable_if_t<!std::is_integral_v<It>>,
            typename = void>
  constexpr vector(It f, It l, memory_type mem)
      : _memory(mem),
        _backend(_memory, f, static_cast<size_type>(std::distance(f, l))) {}

  constexpr vector(const vector &o)
      : _memory(o._memory), _backend(_memory, o._backend) {}

  constexpr vector(const vector &o, memory_type mem)
      : _memory(mem), _backend(_memory, *o._backend) {}

  constexpr vector(vector &&o) noexcept
      : _memory(std::move(o._memory)), _backend(std::move(o._backend)) {}

  constexpr vector(vector &&o, memory_type mem) : _memory(mem), _backend() {
    reserve(o.size());
    for (auto &e : *o._backend)
      _backend.push_back(_memory, std::move(e));
    o.clear();
  }

  constexpr ~vector() = default;

  /* capacity ----------------------------------------------------------- */
  constexpr bool empty() const noexcept { return size() == 0; }
  constexpr size_type size() const noexcept { return _backend.size(); }
  constexpr size_type capacity() const noexcept { return _backend.capacity(); }

  constexpr void reserve(size_type n) { _backend.reserve(_memory, n); }
  constexpr void resize(size_type n) { _backend.resize(_memory, n); }
  constexpr void clear() { _backend.clear(); }

  /* element access ----------------------------------------------------- */
  constexpr auto operator[](size_type i) {
    if constexpr (is_vector<T>::value)
      return view_type(_backend[i], _memory);
    else
      return _backend[i];
  }

  constexpr const auto operator[](size_type i) const {
    if constexpr (is_vector<T>::value)
      return view_type(_backend[i], _memory);
    else
      return _backend[i];
  }

  /* modifiers ---------------------------------------------------------- */
  constexpr void push_back(const T &v) {
    if constexpr (is_vector<T>::value)
      // the first _memory is for the vector itself, the second for the
      // back-end
      _backend.emplace_back(_memory, _memory, *v._backend); // copy back‑end
    else
      _backend.push_back(_memory, v);
  }
  constexpr void push_back(T &&v) {
    if constexpr (is_vector<T>::value)
      _backend.emplace_back(_memory, std::move(*v._backend)); // move back‑end
    else
      _backend.push_back(_memory, std::move(v));
  }

  /*
  template <class... Args> constexpr T &emplace_back(Args &&...args) {
    return *_backend.emplace_back(_memory, std::forward<Args>(args)...);
  }
  */
  template <class... Args> constexpr auto emplace_back(Args &&...args) {
    if constexpr (is_vector<T>::value) {
      auto &inner_ref =
          *_backend.emplace_back(_memory, std::forward<Args>(args)...);
      return value_type(inner_ref, _memory);
    } else {
      return (*_backend.emplace_back(_memory, std::forward<Args>(args)...));
    }
  }

  constexpr iterator insert(iterator pos, const T &val) {
    size_type idx = pos - begin();
    if constexpr (is_vector<T>::value)
      _backend.insert(_memory, _backend.begin() + idx, *val._backend);
    else
      _backend.insert(_memory, _backend.begin() + idx, val);
    return iterator(_backend.begin() + idx, _memory);
  }

  constexpr void pop_back() { _backend.pop_back(); }

  /* iterators ---------------------------------------------------------- */
  constexpr iterator begin() noexcept { return {_backend.begin(), _memory}; }
  constexpr iterator end() noexcept { return {_backend.end(), _memory}; }
  constexpr const_iterator begin() const noexcept {
    return {_backend.begin(), _memory};
  }
  constexpr const_iterator end() const noexcept {
    return {_backend.end(), _memory};
  }
};

} // namespace ycetl
