#pragma once

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <ycetl/impl/container.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

template <typename Key, typename Compare, typename BackendMode, typename Memory>
class set;

template <typename Key, typename Compare = std::less<Key>,
          typename BackendMode = container::by_value,
          typename Memory = typename container::container_traits<
              set, type_set<Key, Compare>>::default_memory>
class set;

template <typename Key, typename Compare, typename BackendMode, typename Memory>
class set
    : public container::container<set, Key, Compare, BackendMode, Memory> {
public:
  using key_type = Key;
  using value_type = Key;
  using key_compare = Compare;

  using base_type =
      container::container<set, Key, Compare, BackendMode, Memory>;
  using traits = typename base_type::traits;

  using storage_unit = typename traits::storage_unit;
  using backend_type = typename traits::backend_type;
  using memory_type = Memory;

  using size_type = typename traits::size_type;

  using iterator = storage_unit *;
  using const_iterator = const storage_unit *;

  using reference = typename traits::reference;
  using const_reference = typename traits::const_reference;
  using pointer = typename traits::pointer;
  using const_pointer = typename traits::const_pointer;

private:
  Memory _memory;
  backend_type _backend;

  key_compare _comp;

public:
  // Internal constructor
  constexpr set(backend_type &backend, Memory &alloc)
      : _memory(&alloc), _backend(&backend), _comp() {}

  /* constructors */
  constexpr set() : _memory(), _backend(), _comp() {}
  explicit constexpr set(Memory &memory)
      : _memory(memory), _backend(), _comp() {}

  constexpr set(std::initializer_list<Key> il, Memory &memory)
      : _memory(memory), _backend() {
    for (const auto &e : il)
      insert(e); // use insert to maintain uniqueness & sorting
  }

  constexpr set(std::initializer_list<Key> il) : _memory(), _backend() {
    for (const auto &e : il)
      insert(e);
  }

  constexpr set(const set &o)
      : _memory(), _backend(_memory, o._backend), _comp(o._comp) {}

  constexpr set(const set &o, Memory &memory)
      : _memory(&memory), _backend(_memory, o._backend), _comp(o._comp) {}

  constexpr set(set &&o) noexcept
      : _memory(std::move(o._memory_ptr)), _backend(std::move(o._backend)),
        _comp(std::move(o._comp)) {}

  constexpr set(set &&o, Memory &memory)
      : _memory(&memory), _backend(), _comp(o._comp) {
    for (auto &e : *o._backend)
      insert(std::move(e));
    o.clear();
  }

  constexpr ~set() = default;

  /* capacity ----------------------------------------------------------- */
  constexpr bool empty() const noexcept { return size() == 0; }
  constexpr size_type size() const noexcept { return _backend.size(); }

  constexpr void clear() { _backend.clear(); }

  /* modifiers ---------------------------------------------------------- */
  constexpr std::pair<iterator, bool> insert(const Key &val) {
    auto pos = std::lower_bound(_backend.begin(), _backend.end(), val, _comp);
    if (pos != _backend.end() && !_comp(val, *pos))
      return {pos, false};

    pos = _backend.insert(_memory, pos, val);
    return {pos, true};
  }

  constexpr std::pair<iterator, bool> insert(Key &&val) {
    auto pos = std::lower_bound(_backend.begin(), _backend.end(), val, _comp);
    if (pos != _backend.end() && !_comp(val, *pos))
      return {pos, false};

    pos = _backend.insert(_memory, pos, std::move(val));
    return {pos, true};
  }

  template <class... Args>
  constexpr std::pair<iterator, bool> emplace(Args &&...args) {
    Key val(std::forward<Args>(args)...);
    return insert(std::move(val));
  }

  /* element access ----------------------------------------------------- */
  constexpr iterator find(const Key &key) {
    auto pos = std::lower_bound(_backend.begin(), _backend.end(), key, _comp);
    if (pos != _backend.end() && !_comp(key, *pos))
      return pos;
    return end();
  }

  constexpr const_iterator find(const Key &key) const {
    auto pos = std::lower_bound(_backend.begin(), _backend.end(), key, _comp);
    if (pos != _backend.end() && !_comp(key, *pos))
      return pos;
    return end();
  }

  constexpr bool contains(const Key &key) const { return find(key) != end(); }

  /* iterators ---------------------------------------------------------- */
  constexpr iterator begin() noexcept { return _backend.begin(); }
  constexpr iterator end() noexcept { return _backend.end(); }

  constexpr const_iterator begin() const noexcept { return _backend.begin(); }
  constexpr const_iterator end() const noexcept { return _backend.end(); }

  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
};

} // namespace ycetl
