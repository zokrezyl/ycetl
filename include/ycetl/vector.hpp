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

template <typename T,
          typename Memory =
              typename container::container_traits<vector, T>::default_memory,
          typename BackendMode = container::by_value>
class vector;

/* detect nested vector --------------------------------------------------- */
template <class> struct is_vector : std::false_type {};
template <class U, class A, class BM>
struct is_vector<vector<U, A, BM>> : std::true_type {};

template <typename T, typename Memory, bool Const, typename BackendMode>
class vector_iterator {
public:
  using iterator_category = std::random_access_iterator_tag;
  using storage_unit = std::conditional_t<is_vector<T>::value, typename T::backend_type, T>;
  using raw_ptr = std::conditional_t<Const, const storage_unit *, storage_unit *>;

  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = std::conditional_t<Const, const value_type *, value_type *>;
  using reference = std::conditional_t<Const, const value_type &, value_type &>;

private:
  raw_ptr _ptr = nullptr;
  [[no_unique_address]] Memory _memory;

public:
  constexpr vector_iterator() = default;
  constexpr vector_iterator(raw_ptr p, Memory mem) : _ptr(p), _memory(mem) {}

  template <bool OtherConst, typename = std::enable_if_t<Const && !OtherConst>>
  constexpr vector_iterator(const vector_iterator<T, Memory, OtherConst, BackendMode> &other)
      : _ptr(other._ptr), _memory(other._memory) {}


  // Modified constructor for better compatibility with dynamic_array iterators.
  template <bool OtherConst>
  constexpr vector_iterator(
      typename ycetl::dynamic_array<storage_unit>::basic_iterator<OtherConst> backend_it,
      Memory mem)
      : _ptr(&*backend_it), _memory(mem) {}


  constexpr reference operator*() const {
    if constexpr (is_vector<T>::value) {
      // For nested vectors, construct a view_type (which is a vector reference)
      // using the underlying backend type reference and the memory.
      return value_type(
          reinterpret_cast<typename T::backend_type &>(const_cast<storage_unit &>(*_ptr)),
          _memory);
    } else {
      return reinterpret_cast<reference>(*_ptr);
    }
  }

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

  friend class vector<T, Memory, BackendMode>;
};

/*行行行行行行行行行行行行行行行行行行行行行行行行行 vector 行行行行行行行行行行行行行行行行行行行行行行行行行*/
template <typename T,
  typename Memory,
  typename BackendMode>
class vector {
public:
  using traits = container::container_traits<vector, T, Memory, BackendMode>;

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

  using iterator = vector_iterator<value_type, memory_type, false, BackendMode>;
  using const_iterator = vector_iterator<value_type, memory_type, true, BackendMode>;

private:
  [[no_unique_address]] memory_type _memory;
  backend_type _backend;

public:
  // Changed constructor for nested vectors to accept backend_type& directly
  template <typename OtherMemory>
  constexpr vector(backend_type& backend_ref, OtherMemory other_memory)
      : _memory(other_memory), _backend(backend_ref) {}

  template <class, class, class> friend class vector;

  constexpr vector() : _memory(), _backend(_memory) {}
  explicit constexpr vector(memory_type mem) : _memory(mem), _backend(mem) {}

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
      : _memory(mem), _backend(_memory, o._backend) {}

  constexpr vector(vector &&o) noexcept
      : _memory(std::move(o._memory)), _backend(std::move(o._backend)) {}

  constexpr vector(vector &&o, memory_type mem) : _memory(mem), _backend(mem) {
    if (this->_memory == o._memory) {
        _backend = std::move(o._backend);
    } else {
        reserve(o.size());
        for (auto &e : o)
          push_back(std::move(e));
        o.clear();
    }
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
      _backend.emplace_back(_memory, v._backend);
    else
      _backend.push_back(_memory, v);
  }
  constexpr void push_back(T &&v) {
    if constexpr (is_vector<T>::value)
      _backend.emplace_back(_memory, std::move(v._backend));
    else
      _backend.push_back(_memory, std::move(v));
  }

  template <class... Args> constexpr auto emplace_back(Args &&...args) {
    if constexpr (is_vector<T>::value) {
      T temp_inner_vec(std::forward<Args>(args)...);
      auto& emplaced_backend_ref = *_backend.emplace_back(_memory, std::move(temp_inner_vec._backend));
      return T(emplaced_backend_ref, _memory);
    } else {
      return (*_backend.emplace_back(_memory, std::forward<Args>(args)...));
    }
  }


  constexpr iterator insert(iterator pos, const T &val) {
    size_type idx = pos - begin();
    if constexpr (is_vector<T>::value)
      _backend.insert(_memory, _backend.begin() + idx, val._backend);
    else
      _backend.insert(_memory, _backend.begin() + idx, val);
    return iterator(_backend.begin() + idx, _memory);
  }

  constexpr void pop_back() { _backend.pop_back(); }

  /* iterators ---------------------------------------------------------- */
  constexpr iterator begin() noexcept { return {_backend.data(), _memory}; }
  constexpr iterator end() noexcept { return {_backend.data() + _backend.size(), _memory}; }
  constexpr const_iterator begin() const noexcept {
    return {_backend.data(), _memory};
  }
  constexpr const_iterator end() const noexcept {
    return {_backend.data() + _backend.size(), _memory};
  }
};

} // namespace ycetl
