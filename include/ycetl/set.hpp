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
          typename Memory = typename container::container<Key>::default_memory>
// clang-format on
class set : public container::container<Key, Memory> {
public:
  using base_type = container::container<Key, Memory>;
  using typename base_type::relevant_of;
  using typename base_type::storage_type;
  using typename base_type::storage_unit;
  using memory_type = Memory;

  using key_type = Key;
  using value_type = Key;
  using size_type = std::size_t;
  using iterator = storage_unit *;
  using const_iterator = const storage_unit *;
  using key_compare = Compare;

private:
  owned_pointer<Memory> _memory_ptr;
  owned_pointer<storage_type> _storage;

  key_compare _comp;

public:
  // Internal constructor
  constexpr set(storage_type &storage, Memory &alloc)
      : _memory_ptr(&alloc), _storage(&storage), _comp() {}

  constexpr Memory &alloc() { return *_memory_ptr; }
  constexpr const Memory &alloc() const { return *_memory_ptr; }

  /* constructors */
  constexpr set() : _memory_ptr(), _storage(), _comp() {}
  explicit constexpr set(Memory &a) : _memory_ptr(&a), _storage(), _comp() {}

  constexpr set(std::initializer_list<Key> il, Memory &a)
      : _memory_ptr(&a), _storage() {
    for (const auto &e : il)
      insert(e); // use insert to maintain uniqueness & sorting
  }

  constexpr set(std::initializer_list<Key> il) : _memory_ptr(), _storage() {
    for (const auto &e : il)
      insert(e);
  }

  constexpr set(const set &o)
      : _memory_ptr(), _storage(alloc(), *o._storage), _comp(o._comp) {}

  constexpr set(const set &o, Memory &a)
      : _memory_ptr(&a), _storage(alloc(), *o._storage), _comp(o._comp) {}

  constexpr set(set &&o) noexcept
      : _memory_ptr(std::move(o._memory_ptr)), _storage(std::move(o._storage)),
        _comp(std::move(o._comp)) {}

  constexpr set(set &&o, Memory &a)
      : _memory_ptr(&a), _storage(), _comp(o._comp) {
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
