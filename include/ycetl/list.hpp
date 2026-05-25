// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace ycetl {

// Doubly-linked list. Nodes are heap-allocated with new/delete — same
// constexpr discipline as ycetl::unique_ptr / shared_ptr (allocate and
// deallocate must balance inside one constant evaluation).
//
// Stable iterators (push_back/push_front/insert/erase don't invalidate
// iterators to other elements), bidirectional iteration. No allocator
// parameter — the per-node lifetime is too small to be worth wiring
// into a multitype memory backend; if you need bulk-typed allocation,
// reach for ycetl::dynamic_array instead.
//
// Replaces the earlier container::container<>-based skeleton in this
// file, which never compiled in tree.
template <typename T> class list {
private:
  struct node {
    T value;
    node *prev;
    node *next;

    template <typename... Args>
    constexpr node(node *p, node *n, Args &&...args)
        : value(std::forward<Args>(args)...), prev(p), next(n) {}
  };

  node *_head = nullptr;
  node *_tail = nullptr;
  std::size_t _size = 0;

  constexpr void link_after(node *prev, node *fresh) {
    node *next = prev ? prev->next : _head;
    fresh->prev = prev;
    fresh->next = next;
    if (prev)
      prev->next = fresh;
    else
      _head = fresh;
    if (next)
      next->prev = fresh;
    else
      _tail = fresh;
  }

  constexpr void unlink(node *n) {
    if (n->prev)
      n->prev->next = n->next;
    else
      _head = n->next;
    if (n->next)
      n->next->prev = n->prev;
    else
      _tail = n->prev;
  }

  constexpr void clear_nodes() {
    node *cur = _head;
    while (cur) {
      node *nx = cur->next;
      delete cur;
      cur = nx;
    }
    _head = _tail = nullptr;
    _size = 0;
  }

public:
  using value_type = T;
  using size_type = std::size_t;

  // Bidirectional iterator. Const-ness applies to what *it returns,
  // not to the iterator's position state — same shape we settled on
  // for dynamic_array's basic_iterator.
  template <bool IsConst> class basic_iter {
    friend class list;
    using node_pointer = std::conditional_t<IsConst, const node *, node *>;
    node_pointer _n = nullptr;
    // We keep a back-pointer to the owning list so that ++end()
    // and --begin() don't crash for empty containers, and so that
    // --end() can land on _tail.
    using list_pointer = std::conditional_t<IsConst, const list *, list *>;
    list_pointer _l = nullptr;

  public:
    using value_type = T;
    using reference_type = std::conditional_t<IsConst, const T &, T &>;
    using pointer_type = std::conditional_t<IsConst, const T *, T *>;

    constexpr basic_iter() = default;
    constexpr basic_iter(list_pointer l, node_pointer n) : _n(n), _l(l) {}

    constexpr operator basic_iter<true>() const
      requires(!IsConst)
    {
      basic_iter<true> r;
      r._n = _n;
      r._l = _l;
      return r;
    }

    constexpr reference_type operator*() const { return _n->value; }
    constexpr pointer_type operator->() const { return &_n->value; }

    constexpr basic_iter &operator++() {
      _n = _n->next;
      return *this;
    }
    constexpr basic_iter operator++(int) {
      auto t = *this;
      ++(*this);
      return t;
    }
    constexpr basic_iter &operator--() {
      _n = _n ? _n->prev : _l->_tail;
      return *this;
    }
    constexpr basic_iter operator--(int) {
      auto t = *this;
      --(*this);
      return t;
    }

    constexpr bool operator==(const basic_iter &o) const { return _n == o._n; }
    constexpr bool operator!=(const basic_iter &o) const { return _n != o._n; }
  };

  using iterator = basic_iter<false>;
  using const_iterator = basic_iter<true>;

  constexpr list() = default;

  constexpr list(std::initializer_list<T> il) {
    for (const auto &v : il)
      push_back(v);
  }

  constexpr list(const list &o) {
    for (auto it = o.begin(); it != o.end(); ++it)
      push_back(*it);
  }

  constexpr list(list &&o) noexcept
      : _head(o._head), _tail(o._tail), _size(o._size) {
    o._head = o._tail = nullptr;
    o._size = 0;
  }

  constexpr list &operator=(const list &o) {
    if (this != &o) {
      clear_nodes();
      for (auto it = o.begin(); it != o.end(); ++it)
        push_back(*it);
    }
    return *this;
  }

  constexpr list &operator=(list &&o) noexcept {
    if (this != &o) {
      clear_nodes();
      _head = o._head;
      _tail = o._tail;
      _size = o._size;
      o._head = o._tail = nullptr;
      o._size = 0;
    }
    return *this;
  }

  constexpr ~list() { clear_nodes(); }

  constexpr size_type size() const noexcept { return _size; }
  constexpr bool empty() const noexcept { return _size == 0; }
  constexpr void clear() { clear_nodes(); }

  constexpr T &front() { return _head->value; }
  constexpr const T &front() const { return _head->value; }
  constexpr T &back() { return _tail->value; }
  constexpr const T &back() const { return _tail->value; }

  constexpr iterator begin() noexcept { return iterator(this, _head); }
  constexpr iterator end() noexcept { return iterator(this, nullptr); }
  constexpr const_iterator begin() const noexcept {
    return const_iterator(this, _head);
  }
  constexpr const_iterator end() const noexcept {
    return const_iterator(this, nullptr);
  }

  constexpr void push_back(const T &v) { emplace_back(v); }
  constexpr void push_back(T &&v) { emplace_back(std::move(v)); }

  template <typename... Args> constexpr T &emplace_back(Args &&...args) {
    node *n = new node(_tail, nullptr, std::forward<Args>(args)...);
    if (_tail)
      _tail->next = n;
    else
      _head = n;
    _tail = n;
    ++_size;
    return n->value;
  }

  constexpr void push_front(const T &v) { emplace_front(v); }
  constexpr void push_front(T &&v) { emplace_front(std::move(v)); }

  template <typename... Args> constexpr T &emplace_front(Args &&...args) {
    node *n = new node(nullptr, _head, std::forward<Args>(args)...);
    if (_head)
      _head->prev = n;
    else
      _tail = n;
    _head = n;
    ++_size;
    return n->value;
  }

  constexpr void pop_back() {
    node *t = _tail;
    unlink(t);
    delete t;
    --_size;
  }
  constexpr void pop_front() {
    node *h = _head;
    unlink(h);
    delete h;
    --_size;
  }

  // Insert before pos. Returns iterator to the new element.
  constexpr iterator insert(iterator pos, const T &v) {
    node *prev = pos._n ? pos._n->prev : _tail;
    node *fresh = new node(nullptr, nullptr, v);
    link_after(prev, fresh);
    ++_size;
    return iterator(this, fresh);
  }

  // Erase pos; returns iterator to the element after.
  constexpr iterator erase(iterator pos) {
    node *next = pos._n->next;
    unlink(pos._n);
    delete pos._n;
    --_size;
    return iterator(this, next);
  }
};

} // namespace ycetl
