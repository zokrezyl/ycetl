#pragma once

#include <memory>
#include <ycetl/memory.hpp>

namespace ycetl {

#if !__cpp_exceptions
// Iff -fno-exceptions, transform error handling code to work without it.
#define __ycetl_try if (true)
#define __ycetl_catch(X) if (false)
#define __ycetl_throw_exception_again
#else
// Else proceed normally.
#define __ycetl_try try
#define __ycetl_catch(X) catch (X)
#define __ycetl_throw_exception_again throw
#endif

// we structure similarily to the C++ standard library's vector
template <typename _Tp, typename _Alloc> struct _vector_base {
  typedef typename std::allocator_traits<_Alloc>::template rebind_alloc<_Tp>
      _Tp_alloc_type;
  typedef typename std::allocator_traits<_Tp_alloc_type>::pointer pointer;

  struct _vector_impl_data {
    pointer _start;
    pointer _finish;
    pointer _end_of_storage;

    constexpr _vector_impl_data() noexcept
        : _start(), _finish(), _end_of_storage() {}
    constexpr _vector_impl_data(_vector_impl_data &&__x) noexcept
        : _start(__x._start), _finish(__x._finish),
          _end_of_storage(__x._end_of_storage) {
      __x._start = __x._finish = __x._end_of_storage = pointer();
    }
    constexpr void _copy_data(_vector_impl_data const &__x) noexcept {
      _start = __x._start;
      _finish = __x._finish;
      _end_of_storage = __x._end_of_storage;
    }
    constexpr void _swap_data(_vector_impl_data &__x) noexcept {
      // Do not use std::swap(_start, __x._start), etc as it loses
      // information used by TBAA.
      _vector_impl_data __tmp;
      __tmp._copy_data(*this);
      _copy_data(__x);
      __x._copy_data(__tmp);
    }
  };

  struct _vector_impl : public _Tp_alloc_type, public _vector_impl_data {
    constexpr _vector_impl() noexcept(
        std::is_nothrow_default_constructible<_Tp_alloc_type>::value)
        : _Tp_alloc_type() {}
    constexpr _vector_impl(_Tp_alloc_type const &__a) noexcept
        : _Tp_alloc_type(__a) {}
    constexpr _vector_impl(_Tp_alloc_type &&__a) noexcept
        : _Tp_alloc_type(std::move(__a)) {}
    constexpr _vector_impl(_Tp_alloc_type &&__a, _vector_impl &&__rv) noexcept
        : _Tp_alloc_type(std::move(__a)), _vector_impl_data(std::move(__rv)) {}
  };

protected:
  constexpr pointer _allocate(size_t _n) {
    if (_n == 0) {
      return pointer();
    }
    typedef std::allocator_traits<_Tp_alloc_type> _AllocTraits;
    return _AllocTraits::allocate(_impl, _n);
  }

  constexpr void _deallocate(pointer __p, size_t __n) {
    typedef std::allocator_traits<_Tp_alloc_type> _AllocTraits;
    if (__p)
      _AllocTraits::deallocate(_impl, __p, __n);
  }

  constexpr void _create_storage(size_t _n) {
    this->_impl._start = this->_allocate(_n);
    this->_impl._finish = this->_impl._start;
    this->_impl._end_of_storage = this->_impl._start + _n;
  }

public:
  typedef _Alloc allocator_type;
  constexpr _Tp_alloc_type &_get_tp_allocator() { return this->_impl; }
  constexpr const _Tp_alloc_type &_get_tp_allocator() const {
    return this->_impl;
  }
  constexpr allocator_type _get_allocator() const {
    return allocator_type(_get_tp_allocator());
  }

  constexpr _vector_base(const allocator_type &__a) : _impl(__a) {}
  constexpr _vector_base(size_t _n) : _impl() { _create_storage(_n); }

  constexpr _vector_base() : _impl() {};

  constexpr _vector_base(size_t __n, const allocator_type &__a) : _impl(__a) {
    _create_storage(__n);
  }

  // layout
public:
  _vector_impl _impl;
};

constexpr inline void __ycetl__throw_length_error(const char *msg) {
  // throw std::length_error(msg);
}

template <typename _Tp, typename _Alloc>
class vector : public _vector_base<_Tp, _Alloc> {

  typedef _vector_base<_Tp, _Alloc> _Base;
  typedef typename _Base::_Tp_alloc_type _tp_alloc_type;
  typedef std::allocator_traits<_tp_alloc_type> _Alloc_traits;

public:
  typedef _Tp value_type;
  typedef typename _Base::pointer pointer;
  typedef typename _Alloc_traits::const_pointer const_pointer;
  typedef value_type &reference;
  typedef const value_type &const_reference;

  typedef __gnu_cxx::__normal_iterator<pointer, vector> iterator;
  typedef __gnu_cxx::__normal_iterator<const_pointer, vector> const_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef _Alloc allocator_type;

  // type interface for self
protected:
  using _Base::_allocate;
  using _Base::_deallocate;
  using _Base::_get_allocator;
  using _Base::_get_tp_allocator;
  using _Base::_impl;

public:
  constexpr vector() : _Base() {};
  constexpr vector(const allocator_type &__a) noexcept : _Base(__a) {}

  explicit constexpr vector(size_type __n,
                            const allocator_type &__a = allocator_type())
      : _Base(_check_init_len(__n, __a), __a) {
    _default_initialize(__n);
  }

  constexpr vector(size_type __n, const value_type &__value,
                   const allocator_type &__a = allocator_type())
      : _Base(_check_init_len(__n, __a), __a) {
    _fill_initialize(__n, __value);
  }

  constexpr vector(const vector &__x)
      : _Base(__x.size(), _Alloc_traits::select_on_container_copy_construction(
                              __x._get_tp_allocator())) {
    this->_impl._finish = ycetl::memory::uninitialized_copy(
        __x.begin(), __x.end(), this->_impl._start, _get_tp_allocator());
  }

  constexpr vector(vector &&) noexcept = default;

  constexpr vector(std::initializer_list<value_type> __l,
                   const allocator_type &__a = allocator_type())
      : _Base(__a) {

    _range_initialize(__l.begin(), __l.end(),
                      std::random_access_iterator_tag());
  }

  template <typename _InputIterator,
            typename = std::_RequireInputIter<_InputIterator>>
  vector(_InputIterator __first, _InputIterator __last,
         const allocator_type &__a = allocator_type())
      : _Base(__a) {
    _range_initialize(__first, __last, std::__iterator_category(__first));
  }

  constexpr ~vector() noexcept {
    ycetl::memory::destroy(this->_impl._start, this->_impl._finish,
                           _get_tp_allocator());
  }

  constexpr vector &operator=(const vector &__x) noexcept(
      std::allocator_traits<
          _Alloc>::propagate_on_container_copy_assignment::value ||
      std::allocator_traits<_Alloc>::is_always_equal::value) {

    constexpr bool __move_storage =
        std::allocator_traits<
            _Alloc>::propagate_on_container_copy_assignment::value ||
        std::allocator_traits<_Alloc>::is_always_equal::value;
    _move_assign(std::move(__x), std::bool_constant<__move_storage>());
    return *this;
  }

  constexpr vector &operator=(std::initializer_list<value_type> __l) {
    this->_assign_aux(__l.begin(), __l.end(),
                      std::random_access_iterator_tag());
    return *this;
  }

  constexpr const_iterator begin() const noexcept {
    return const_iterator(this->_impl._start);
  }

  const_iterator cbegin() const noexcept {
    return const_iterator(this->_impl._start);
  }

  constexpr iterator begin() noexcept { return iterator(this->_impl._start); }

  template <typename... _Args> reference emplace_back(_Args &&...__args);

  constexpr void push_back(const value_type &__x) {
    if (this->_impl._finish != this->_impl._end_of_storage) {
      _Alloc_traits::construct(_get_tp_allocator(), this->_impl._finish, __x);
      ++this->_impl._finish;
    } else {
      _realloc_insert(end(), __x);
    }
  }

  constexpr size_type max_size() const noexcept {
    return _max_size(_get_tp_allocator());
  }

  constexpr size_type size() const _GLIBCXX_NOEXCEPT {
    return size_type(this->_impl._finish - this->_impl._start);
  }

  constexpr void clear() noexcept { _erase_at_end(this->_impl._start); }

  constexpr iterator end() noexcept { return iterator(this->_impl._finish); }
  constexpr const_iterator end() const noexcept {
    return iterator(this->_impl._finish);
  }

  constexpr void reserve(size_type __n);

  constexpr reference back() noexcept { return *(end() - 1); }

  /**
   *  Returns a read-only (constant) reference to the data at the
   *  last element of the %vector.
   */
  constexpr const_reference back() const noexcept { return *(end() - 1); }

  [[__nodiscard__]] constexpr const_iterator cend() const noexcept {
    return const_iterator(this->_impl._finish);
  }

  constexpr size_type capacity() const noexcept {
    return size_type(this->_impl._end_of_storage - this->_impl._start);
  }

  constexpr iterator insert(const_iterator __position, value_type &&__x) {
    return _insert_rval(__position, std::move(__x));
  }
  constexpr reference operator[](size_type __n) noexcept {
    return *(this->_impl._start + __n);
  }

  constexpr const_reference operator[](size_type __n) const noexcept {
    return *(this->_impl._start + __n);
  }

private:
  constexpr void _default_initialize(size_type __n) {
    this->_impl._finish = std::__uninitialized_default_n_a(
        this->_impl._start, __n, _get_tp_allocator());
  }

  constexpr void _fill_initialize(size_type __n, const value_type &__value) {
    this->_impl._finish = std::__uninitialized_fill_n_a(
        this->_impl._start, __n, __value, _get_tp_allocator());
  }

  static constexpr size_type _check_init_len(size_type __n,
                                             const allocator_type &__a) {
    if (__n > _max_size(_tp_alloc_type(__a)))
      // TODO
      __ycetl__throw_length_error(
          "cannot create std::vector larger than max_size()");
    return __n;
  }

  constexpr void _erase_at_end(pointer __pos) noexcept {
    if (size_type __n = this->_impl._finish - __pos) {
      ycetl::memory::destroy(__pos, this->_impl._finish, _get_tp_allocator());
      this->_impl._finish = __pos;
    }
  }

  template <typename _InputIterator>
  constexpr void _range_initialize(_InputIterator __first,
                                   _InputIterator __last,
                                   std::input_iterator_tag) {
    __ycetl_try {
      for (; __first != __last; ++__first)
        emplace_back(*__first);
    }
    __ycetl_catch(...) {
      clear();
      __ycetl_throw_exception_again;
    }
  }

  template <typename _ForwardIterator>
  constexpr void _range_initialize(_ForwardIterator __first,
                                   _ForwardIterator __last,
                                   std::forward_iterator_tag) {
    const size_type __n = std::distance(__first, __last);
    this->_impl._start =
        this->_allocate(_check_init_len(__n, _get_tp_allocator()));
    this->_impl._end_of_storage = this->_impl._start + __n;
    this->_impl._finish = ycetl::memory::uninitialized_copy(
        __first, __last, this->_impl._start, _get_tp_allocator());
  }

  template <typename _InputIterator>
  void _range_insert(iterator __pos, _InputIterator __first,
                     _InputIterator __last, std::input_iterator_tag);

  template <typename... _Args>
  constexpr void _realloc_insert(iterator __position, _Args &&...__args);

  template <typename _Arg>
  constexpr void _insert_aux(iterator __position, _Arg &&__arg);

  // Either move-construct at the end, or forward to _M_insert_aux.
  constexpr iterator _insert_rval(const_iterator __position, value_type &&__v);

  template <typename _InputIterator>
  constexpr void _assign_aux(_InputIterator __first, _InputIterator __last,
                             std::input_iterator_tag);

  template <typename _ForwardIterator>
  constexpr void _assign_aux(_ForwardIterator __first, _ForwardIterator __last,
                             std::forward_iterator_tag);

  // Try to emplace at the end, otherwise forward to _M_insert_aux.
  template <typename... _Args>
  constexpr iterator _emplace_aux(const_iterator __position, _Args &&...__args);

  // Emplacing an rvalue of the correct type can use _M_insert_rval.
  constexpr iterator _emplace_aux(const_iterator __position, value_type &&__v) {
    return _insert_rval(__position, std::move(__v));
  }

  constexpr size_type _check_len(size_type __n, const char *__s) const {
    if (max_size() - size() < __n)
      __ycetl__throw_length_error(__s);

    const size_type __len = size() + (std::max)(size(), __n);
    return (__len < size() || __len > max_size()) ? max_size() : __len;
  }

  static constexpr size_type _max_size(const _tp_alloc_type &__a) noexcept {
    // std::distance(begin(), end()) cannot be greater than PTRDIFF_MAX,
    // and realistically we can't store more than PTRDIFF_MAX/sizeof(T)
    // (even if std::allocator_traits::max_size says we can).
    const size_t __diffmax =
        __gnu_cxx::__numeric_traits<ptrdiff_t>::__max / sizeof(_Tp);
    const size_t __allocmax = _Alloc_traits::max_size(__a);
    return (std::min)(__diffmax, __allocmax);
  }

  static constexpr bool _use_relocate() {
    // Instantiating std::__relocate_a might cause an error outside the
    // immediate context (in __relocate_object_a's noexcept-specifier),
    // so only do it if we know the type can be move-inserted into *this.
    // return _S_nothrow_relocate(__is_move_insertable<_Tp_alloc_type>{});
    return std::is_nothrow_constructible<value_type, value_type &&>::value;
  }

  static constexpr bool _nothrow_relocate(std::true_type) {
    /*
    return noexcept(std::__relocate_a(
        std::declval<pointer>(), std::declval<pointer>(),
        std::declval<pointer>(), std::declval<_Tp_alloc_type &>()));
    */

    return std::is_nothrow_constructible<value_type, value_type &&>::value;
  }

  static constexpr pointer _relocate(pointer __first, pointer __last,
                                     pointer __result,
                                     _tp_alloc_type &__alloc) noexcept {
    return ycetl::memory::uninitialized_move(__first, __last, __result,
                                             __alloc);
  }

  constexpr void _move_assign(vector &&__x, std::true_type) noexcept {
    vector __tmp(_get_allocator());
    this->_impl._swap_data(__x._impl);
    __tmp._impl._swap_data(__x._impl);
    std::__alloc_on_move(_get_tp_allocator(), __x._get_tp_allocator());
  }

  struct _temporary_value {
    template <typename... _Args>
    constexpr explicit _temporary_value(vector *__vec, _Args &&...__args)
        : _this(__vec) {
      _Alloc_traits::construct(_this->_impl, _ptr(),
                               std::forward<_Args>(__args)...);
    }

    ~_temporary_value() { _Alloc_traits::destroy(_this->_impl, _ptr()); }

    constexpr value_type &_val() noexcept { return _storage._val; }

  private:
    constexpr _Tp *_ptr() noexcept { return std::addressof(_storage._val); }

    union _Storage {
      constexpr _Storage() : _byte() {}
      constexpr ~_Storage() {}
      _Storage &operator=(const _Storage &) = delete;
      unsigned char _byte;
      _Tp _val;
    };
    vector *_this;
    _Storage _storage;
  };

  template <typename _ForwardIterator>
  constexpr pointer _allocate_and_copy(size_type __n, _ForwardIterator __first,
                                       _ForwardIterator __last) {
    pointer __result = this->_allocate(__n);
    __ycetl_try {
      ycetl::memory::uninitialized_copy(__first, __last, __result,
                                        _get_tp_allocator());
      return __result;
    }
    __ycetl_catch(...) {
      _deallocate(__result, __n);
      __throw_exception_again;
    }
  }
};
template <typename _Tp, typename _Alloc>
template <typename... _Args>
constexpr void vector<_Tp, _Alloc>::_realloc_insert(iterator __position,
                                                    _Args &&...__args) {
  const size_type __len = _check_len(size_type(1), "vector::_M_realloc_insert");
  pointer __old_start = this->_impl._start;
  pointer __old_finish = this->_impl._finish;
  const size_type __elems_before = __position - begin();
  pointer __new_start(this->_allocate(__len));
  pointer __new_finish(__new_start);
  __ycetl_try {
    // The order of the three operations is dictated by the C++11
    // case, where the moves could alter a new element belonging
    // to the existing vector.  This is an issue only for callers
    // taking the element by lvalue ref (see last bullet of C++11
    // [res.on.arguments]).
    _Alloc_traits::construct(this->_impl, __new_start + __elems_before,
                             std::forward<_Args>(__args)...);
    __new_finish = pointer();

    if constexpr (_use_relocate()) {
      __new_finish = _relocate(__old_start, __position.base(), __new_start,
                               _get_tp_allocator());

      ++__new_finish;

      __new_finish = _relocate(__position.base(), __old_finish, __new_finish,
                               _get_tp_allocator());
    } else {
      __new_finish = std::__uninitialized_move_if_noexcept_a(
          __old_start, __position.base(), __new_start, _get_tp_allocator());

      ++__new_finish;

      __new_finish = std::__uninitialized_move_if_noexcept_a(
          __position.base(), __old_finish, __new_finish, _get_tp_allocator());
    }
  }
  __ycetl_catch(...) {
    if (!__new_finish)
      _Alloc_traits::destroy(this->_impl, __new_start + __elems_before);
    else
      ycetl::memory::destroy(__new_start, __new_finish, _get_tp_allocator());
    _deallocate(__new_start, __len);
    __ycetl_throw_exception_again;
  }
  if constexpr (!_use_relocate())
    std::destroy(__old_start, __old_finish, _get_tp_allocator());
  _deallocate(__old_start, this->_impl._end_of_storage - __old_start);
  this->_impl._start = __new_start;
  this->_impl._finish = __new_finish;
  this->_impl._end_of_storage = __new_start + __len;
}

template <typename _Tp, typename _Alloc>
template <typename _InputIterator>
constexpr void vector<_Tp, _Alloc>::_assign_aux(_InputIterator __first,
                                                _InputIterator __last,
                                                std::input_iterator_tag) {
  pointer __cur(this->_impl._start);
  for (; __first != __last && __cur != this->_impl._finish;
       ++__cur, (void)++__first)
    *__cur = *__first;
  if (__first == __last)
    _erase_at_end(__cur);
  else
    _range_insert(end(), __first, __last, std::__iterator_category(__first));
}

template <typename _Tp, typename _Alloc>
template <typename _ForwardIterator>
constexpr void vector<_Tp, _Alloc>::_assign_aux(_ForwardIterator __first,
                                                _ForwardIterator __last,
                                                std::forward_iterator_tag) {
  const size_type __len = std::distance(__first, __last);

  if (__len > capacity()) {
    _check_init_len(__len, _get_tp_allocator());
    pointer __tmp(_allocate_and_copy(__len, __first, __last));
    ycetl::memory::destroy(this->_impl._start, this->_impl._finish,
                           _get_tp_allocator());
    _deallocate(this->_impl._start,
                this->_impl._end_of_storage - this->_impl._start);
    this->_impl._start = __tmp;
    this->_impl._finish = this->_impl._start + __len;
    this->_impl._end_of_storage = this->_impl._finish;
  } else if (size() >= __len)
    _erase_at_end(std::copy(__first, __last, this->_impl._start));
  else {
    _ForwardIterator __mid = __first;
    std::advance(__mid, size());
    std::copy(__first, __mid, this->_impl._start);
    const size_type __attribute__((__unused__)) __n = __len - size();
    this->_impl._finish = std::__uninitialized_copy_a(
        __mid, __last, this->_impl._finish, _get_tp_allocator());
  }
}

template <typename _Tp, typename _Alloc>
template <typename _InputIterator>
void vector<_Tp, _Alloc>::_range_insert(iterator __pos, _InputIterator __first,
                                        _InputIterator __last,
                                        std::input_iterator_tag) {
  if (__pos == end()) {
    for (; __first != __last; ++__first)
      insert(end(), *__first);
  } else if (__first != __last) {
    vector __tmp(__first, __last, _get_tp_allocator());
    insert(__pos, std::make_move_iterator(__tmp.begin()),
           std::make_move_iterator(__tmp.end()));
  }
}

template <typename _Tp, typename _Alloc>
constexpr auto vector<_Tp, _Alloc>::_insert_rval(const_iterator __position,
                                                 value_type &&__v) -> iterator {
  const auto __n = __position - cbegin();
  if (this->_impl._finish != this->_impl._end_of_storage)
    if (__position == cend()) {
      _Alloc_traits::construct(this->_impl, this->_impl._finish,
                               std::move(__v));
      ++this->_impl._finish;
    } else
      _insert_aux(begin() + __n, std::move(__v));
  else
    _realloc_insert(begin() + __n, std::move(__v));

  return iterator(this->_impl._start + __n);
}

template <typename _Tp, typename _Alloc>
template <typename... _Args>
constexpr auto vector<_Tp, _Alloc>::_emplace_aux(const_iterator __position,
                                                 _Args &&...__args)
    -> iterator {
  const auto __n = __position - cbegin();
  if (this->_impl._finish != this->_impl._end_of_storage)
    if (__position == cend()) {
      _Alloc_traits::construct(this->_impl, this->_impl._finish,
                               std::forward<_Args>(__args)...);
      ++this->_impl._finish;
    } else {
      // We need to construct a temporary because something in __args...
      // could alias one of the elements of the container and so we
      // need to use it before _M_insert_aux moves elements around.
      _temporary_value __tmp(this, std::forward<_Args>(__args)...);
      _insert_aux(begin() + __n, std::move(__tmp._val()));
    }
  else
    _realloc_insert(begin() + __n, std::forward<_Args>(__args)...);

  return iterator(this->_impl._start + __n);
}

template <typename _Tp, typename _Alloc>
template <typename _Arg>
constexpr void vector<_Tp, _Alloc>::_insert_aux(iterator __position,
                                                _Arg &&__arg) {
  _Alloc_traits::construct(this->_impl, this->_impl._finish,
                           std::move(*(this->_impl._finish - 1)));
  ++this->_impl._finish;

  std::move_backward(__position.base(), this->_impl._finish - 2,
                     this->_impl._finish - 1);
  *__position = std::forward<_Arg>(__arg);
}

template <typename _Tp, typename _Alloc>
template <typename... _Args>
typename vector<_Tp, _Alloc>::reference
vector<_Tp, _Alloc>::emplace_back(_Args &&...__args) {
  if (this->_impl._finish != this->_impl._end_of_storage) {
    _Alloc_traits::construct(this->_impl, this->_impl._finish,
                             std::forward<_Args>(__args)...);
    ++this->_impl._finish;
  } else
    _realloc_insert(end(), std::forward<_Args>(__args)...);
  return back();
}

template <typename _Tp, typename _Alloc>
constexpr void vector<_Tp, _Alloc>::reserve(size_type __n) {
  if (__n > this->max_size())
    throw std::length_error("vector::reserve");

  if (this->capacity() < __n) {
    const size_type __old_size = size();
    pointer __tmp;

    // Simple approach for constexpr - use allocate and copy
    __tmp = this->_allocate(__n);

    // Move elements to new storage
    ycetl::memory::uninitialized_move_if_noexcept(
        this->_impl._start, this->_impl._finish, __tmp, _get_tp_allocator());

    // Destroy old elements
    ycetl::memory::destroy(this->_impl._start, this->_impl._finish,
                           _get_tp_allocator());

    // Deallocate old storage
    this->_deallocate(this->_impl._start,
                      this->_impl._end_of_storage - this->_impl._start);

    // Update pointers
    this->_impl._start = __tmp;
    this->_impl._finish = __tmp + __old_size;
    this->_impl._end_of_storage = this->_impl._start + __n;
  }
}

} // namespace ycetl
