#pragma once

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <ycetl/impl/container.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

/*─────────────────────────────── list ──────────────────────────────────*/
// clang-format off
template <typename T,
          typename Allocator = typename container::container<T>::default_allocator>
// clang-format on
class list : public container::container<T, Allocator> {
public:
  using base_type = container::container<T, Allocator>;
  using typename base_type::relevant_of;
  using typename base_type::storage_type;
  using typename base_type::storage_unit;
  using allocator_type = Allocator;

  using value_type = T;
  using size_type = std::size_t;
  using iterator = storage_unit *;
  using const_iterator = const storage_unit *;

private:
  Memory _memory;
  storage_type _storage;

public:
  // Internal constructor
  constexpr list(storage_type &storage, Allocator &alloc)
      : _alloc_ptr(&alloc), _storage(&storage) {}

  constexpr Allocator &alloc() { return *_alloc_ptr; }
  constexpr const Allocator &alloc() const { return *_alloc_ptr; }

  /* constructors */
  constexpr list() : _alloc_ptr(), _storage() {}
  explicit constexpr list(Allocator &a) : _alloc_ptr(&a), _storage() {}

  constexpr list(size_type n, const T &v)
      : _alloc_ptr(), _storage(alloc(), n, v) {}

  constexpr list(size_type n, const T &v, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), n, v) {}

  explicit constexpr list(size_type n) : _alloc_ptr(), _storage(alloc(), n) {}

  constexpr list(size_type n, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), n) {}

  constexpr list(std::initializer_list<T> il)
      : _alloc_ptr(), _storage(alloc(), il) {}

  constexpr list(std::initializer_list<T> il, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), il) {}

  constexpr list(const list &o)
      : _alloc_ptr(), _storage(alloc(), *o._storage) {}

  constexpr list(const list &o, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), *o._storage) {}

  constexpr list(list &&o) noexcept
      : _alloc_ptr(std::move(o._alloc_ptr)), _storage(std::move(o._storage)) {}

  constexpr list(list &&o, Allocator &a) : _alloc_ptr(&a), _storage() {
    for (auto &e : *o._storage)
      push_back(std::move(e));
    o.clear();
  }

  constexpr ~list() = default;

  /* capacity ----------------------------------------------------------- */
  constexpr bool empty() const noexcept { return size() == 0; }
  constexpr size_type size() const noexcept { return _storage->size(); }

  constexpr void clear() { _storage->clear(); }

  /* modifiers ---------------------------------------------------------- */
  constexpr void push_back(const T &val) { _storage->push_back(alloc(), val); }

  constexpr void push_back(T &&val) {
    _storage->push_back(alloc(), std::move(val));
  }

  template <class... Args> constexpr T &emplace_back(Args &&...args) {
    return *_storage->emplace_back(alloc(), std::forward<Args>(args)...);
  }

  constexpr void pop_back() { _storage->pop_back(); }

  constexpr void push_front(const T &val) {
    _storage->push_front(alloc(), val);
  }

  constexpr void push_front(T &&val) {
    _storage->push_front(alloc(), std::move(val));
  }

  template <class... Args> constexpr T &emplace_front(Args &&...args) {
    return *_storage->emplace_front(alloc(), std::forward<Args>(args)...);
  }

  constexpr void pop_front() { _storage->pop_front(); }

  /* element access ----------------------------------------------------- */
  constexpr T &front() noexcept { return _storage->front(); }
  constexpr const T &front() const noexcept { return _storage->front(); }

  constexpr T &back() noexcept { return _storage->back(); }
  constexpr const T &back() const noexcept { return _storage->back(); }

  /* iterators ---------------------------------------------------------- */
  constexpr iterator begin() noexcept { return _storage->begin(); }
  constexpr iterator end() noexcept { return _storage->end(); }

  constexpr const_iterator begin() const noexcept { return _storage->begin(); }
  constexpr const_iterator end() const noexcept { return _storage->end(); }

  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
};

} // namespace ycetl
