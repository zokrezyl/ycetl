#pragma once

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <ycetl/impl/container.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

/*──────────────────────────── unordered_map ────────────────────────────*/
// clang-format off
template <typename Key,
          typename T,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = typename container::container<std::pair<const Key, T>>::default_allocator>
// clang-format on
class unordered_map
    : public container::container<std::pair<const Key, T>, Allocator> {
public:
  using base_type = container::container<std::pair<const Key, T>, Allocator>;
  using typename base_type::relevant_of;
  using typename base_type::storage_type;
  using typename base_type::storage_unit;
  using allocator_type = Allocator;

  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<const Key, T>;
  using size_type = std::size_t;
  using iterator = storage_unit *;
  using const_iterator = const storage_unit *;
  using hasher = Hash;
  using key_equal = KeyEqual;

private:
  owned_pointer<Allocator> _alloc_ptr;
  owned_pointer<storage_type> _storage;

  hasher _hasher;
  key_equal _key_equal;

public:
  // Internal constructor
  constexpr unordered_map(storage_type &storage, Allocator &alloc)
      : _alloc_ptr(&alloc), _storage(&storage), _hasher(), _key_equal() {}

  constexpr Allocator &alloc() { return *_alloc_ptr; }
  constexpr const Allocator &alloc() const { return *_alloc_ptr; }

  /* constructors */
  constexpr unordered_map()
      : _alloc_ptr(), _storage(), _hasher(), _key_equal() {}
  explicit constexpr unordered_map(Allocator &a)
      : _alloc_ptr(&a), _storage(), _hasher(), _key_equal() {}

  constexpr unordered_map(std::initializer_list<value_type> il)
      : _alloc_ptr(), _storage(alloc(), il), _hasher(), _key_equal() {}

  constexpr unordered_map(std::initializer_list<value_type> il, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), il), _hasher(), _key_equal() {}

  constexpr unordered_map(const unordered_map &o)
      : _alloc_ptr(), _storage(alloc(), *o._storage), _hasher(o._hasher),
        _key_equal(o._key_equal) {}

  constexpr unordered_map(const unordered_map &o, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), *o._storage), _hasher(o._hasher),
        _key_equal(o._key_equal) {}

  constexpr unordered_map(unordered_map &&o) noexcept
      : _alloc_ptr(std::move(o._alloc_ptr)), _storage(std::move(o._storage)),
        _hasher(std::move(o._hasher)), _key_equal(std::move(o._key_equal)) {}

  constexpr unordered_map(unordered_map &&o, Allocator &a)
      : _alloc_ptr(&a), _storage(), _hasher(o._hasher),
        _key_equal(o._key_equal) {
    for (auto &e : *o._storage)
      insert(std::move(e));
    o.clear();
  }

  constexpr ~unordered_map() = default;

  /* capacity ----------------------------------------------------------- */
  constexpr bool empty() const noexcept { return size() == 0; }
  constexpr size_type size() const noexcept { return _storage->size(); }

  constexpr void clear() { _storage->clear(); }

  /* modifiers ---------------------------------------------------------- */
  constexpr std::pair<iterator, bool> insert(const value_type &val) {
    for (auto it = _storage->begin(); it != _storage->end(); ++it) {
      if (_key_equal(it->first, val.first))
        return {it, false};
    }
    auto pos = _storage->insert(alloc(), _storage->end(), val);
    return {pos, true};
  }

  constexpr std::pair<iterator, bool> insert(value_type &&val) {
    for (auto it = _storage->begin(); it != _storage->end(); ++it) {
      if (_key_equal(it->first, val.first))
        return {it, false};
    }
    auto pos = _storage->insert(alloc(), _storage->end(), std::move(val));
    return {pos, true};
  }

  template <class... Args>
  constexpr std::pair<iterator, bool> emplace(Args &&...args) {
    value_type val(std::forward<Args>(args)...);
    return insert(std::move(val));
  }

  /* element access ----------------------------------------------------- */
  constexpr T &operator[](const Key &key) {
    auto it = find(key);
    if (it != end())
      return it->second;
    return insert({key, T()}).first->second;
  }

  constexpr iterator find(const Key &key) {
    for (auto it = begin(); it != end(); ++it) {
      if (_key_equal(it->first, key))
        return it;
    }
    return end();
  }

  constexpr const_iterator find(const Key &key) const {
    for (auto it = begin(); it != end(); ++it) {
      if (_key_equal(it->first, key))
        return it;
    }
    return end();
  }

  constexpr bool contains(const Key &key) const { return find(key) != end(); }

  /* iterators ---------------------------------------------------------- */
  constexpr iterator begin() noexcept { return _storage->begin(); }
  constexpr iterator end() noexcept { return _storage->end(); }

  constexpr const_iterator begin() const noexcept { return _storage->begin(); }
  constexpr const_iterator end() const noexcept { return _storage->end(); }

  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
};

} // namespace ycetl
