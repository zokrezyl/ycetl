#pragma once

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <ycetl/impl/container.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

/*─────────────────────────────── set ──────────────────────────────────*/
// clang-format off
template <typename Key,
          typename Compare = std::less<Key>,
          typename Allocator = typename container::container<Key>::default_allocator>
// clang-format on
class set : public container::container<Key, Allocator> {
public:
  using base_type = container::container<Key, Allocator>;
  using typename base_type::relevant_of;
  using typename base_type::storage_type;
  using typename base_type::storage_unit;
  using allocator_type = Allocator;

  using key_type = Key;
  using value_type = Key;
  using size_type = std::size_t;
  using iterator = storage_unit *;
  using const_iterator = const storage_unit *;
  using key_compare = Compare;

private:
  owned_pointer<Allocator> _alloc_ptr;
  owned_pointer<storage_type> _storage;

  key_compare _comp;

public:
  // Internal constructor
  constexpr set(storage_type &storage, Allocator &alloc)
      : _alloc_ptr(&alloc), _storage(&storage), _comp() {}

  constexpr Allocator &alloc() { return *_alloc_ptr; }
  constexpr const Allocator &alloc() const { return *_alloc_ptr; }

  /* constructors */
  constexpr set() : _alloc_ptr(), _storage(), _comp() {}
  explicit constexpr set(Allocator &a) : _alloc_ptr(&a), _storage(), _comp() {}

  constexpr set(std::initializer_list<Key> il, Allocator &a)
      : _alloc_ptr(&a), _storage() {
    for (const auto &e : il)
      insert(e); // use insert to maintain uniqueness & sorting
  }

  constexpr set(std::initializer_list<Key> il) : _alloc_ptr(), _storage() {
    for (const auto &e : il)
      insert(e);
  }

  constexpr set(const set &o)
      : _alloc_ptr(), _storage(alloc(), *o._storage), _comp(o._comp) {}

  constexpr set(const set &o, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), *o._storage), _comp(o._comp) {}

  constexpr set(set &&o) noexcept
      : _alloc_ptr(std::move(o._alloc_ptr)), _storage(std::move(o._storage)),
        _comp(std::move(o._comp)) {}

  constexpr set(set &&o, Allocator &a)
      : _alloc_ptr(&a), _storage(), _comp(o._comp) {
    for (auto &e : *o._storage)
      insert(std::move(e));
    o.clear();
  }

  constexpr ~set() = default;

  /* capacity ----------------------------------------------------------- */
  constexpr bool empty() const noexcept { return size() == 0; }
  constexpr size_type size() const noexcept { return _storage->size(); }

  constexpr void clear() { _storage->clear(); }

  /* modifiers ---------------------------------------------------------- */
  constexpr std::pair<iterator, bool> insert(const Key &val) {
    auto pos = std::lower_bound(_storage->begin(), _storage->end(), val, _comp);
    if (pos != _storage->end() && !_comp(val, *pos))
      return {pos, false};

    pos = _storage->insert(alloc(), pos, val);
    return {pos, true};
  }

  constexpr std::pair<iterator, bool> insert(Key &&val) {
    auto pos = std::lower_bound(_storage->begin(), _storage->end(), val, _comp);
    if (pos != _storage->end() && !_comp(val, *pos))
      return {pos, false};

    pos = _storage->insert(alloc(), pos, std::move(val));
    return {pos, true};
  }

  template <class... Args>
  constexpr std::pair<iterator, bool> emplace(Args &&...args) {
    Key val(std::forward<Args>(args)...);
    return insert(std::move(val));
  }

  /* element access ----------------------------------------------------- */
  constexpr iterator find(const Key &key) {
    auto pos = std::lower_bound(_storage->begin(), _storage->end(), key, _comp);
    if (pos != _storage->end() && !_comp(key, *pos))
      return pos;
    return end();
  }

  constexpr const_iterator find(const Key &key) const {
    auto pos = std::lower_bound(_storage->begin(), _storage->end(), key, _comp);
    if (pos != _storage->end() && !_comp(key, *pos))
      return pos;
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
