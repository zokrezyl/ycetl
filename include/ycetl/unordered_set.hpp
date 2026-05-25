// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/hash.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

// Open-addressing hash set. Same shape as ycetl::unordered_map: slot
// states are empty/filled/tombstone, linear probe, rehash at 70%
// occupancy. Replaces the earlier flat O(n)-lookup version.
template <typename Key,
          // ycetl::hash, not std::hash — see unordered_map.hpp for why.
          typename Hash = hash<Key>, typename KeyEqual = std::equal_to<Key>>
class unordered_set {
public:
  using key_type = Key;
  using value_type = Key;
  using size_type = std::size_t;
  using hasher = Hash;
  using key_equal = KeyEqual;

private:
  enum class slot_state : std::uint8_t { empty = 0, filled = 1, tombstone = 2 };

  struct slot {
    slot_state state = slot_state::empty;
    Key key{};
  };

  using storage_type = dynamic_array<slot>;

  static constexpr size_type initial_capacity = 8;
  static constexpr size_type max_load_pct = 70;

  storage_type _slots;
  size_type _size = 0;
  hasher _hasher;
  key_equal _eq;

  constexpr size_type bucket_of(const Key &k) const {
    return _hasher(k) % _slots.size();
  }

  constexpr void rehash(size_type new_cap) {
    storage_type new_slots;
    new_slots.resize(new_cap);
    for (size_type i = 0; i < _slots.size(); ++i) {
      if (_slots[i].state != slot_state::filled)
        continue;
      size_type h = _hasher(_slots[i].key) % new_cap;
      for (size_type probe = 0; probe < new_cap; ++probe) {
        size_type j = (h + probe) % new_cap;
        if (new_slots[j].state == slot_state::empty) {
          new_slots[j].key = std::move(_slots[i].key);
          new_slots[j].state = slot_state::filled;
          break;
        }
      }
    }
    _slots = std::move(new_slots);
  }

  constexpr void maybe_grow() {
    if (_slots.size() == 0) {
      _slots.resize(initial_capacity);
      return;
    }
    if ((_size + 1) * 100 > _slots.size() * max_load_pct) {
      rehash(_slots.size() * 2);
    }
  }

  constexpr size_type find_slot(const Key &k) const {
    if (_slots.size() == 0)
      return size_type(-1);
    size_type cap = _slots.size();
    size_type h = bucket_of(k);
    for (size_type probe = 0; probe < cap; ++probe) {
      size_type i = (h + probe) % cap;
      const slot &s = _slots[i];
      if (s.state == slot_state::empty)
        return size_type(-1);
      if (s.state == slot_state::filled && _eq(s.key, k))
        return i;
    }
    return size_type(-1);
  }

  template <bool IsConst> class basic_iter {
    friend class unordered_set;
    using container_pointer =
        std::conditional_t<IsConst, const unordered_set *, unordered_set *>;
    container_pointer _s = nullptr;
    size_type _i = 0;

    constexpr void advance_to_filled() {
      while (_s && _i < _s->_slots.size()
             && _s->_slots[_i].state != slot_state::filled)
        ++_i;
    }

  public:
    using value_type = Key;
    using reference_type = std::conditional_t<IsConst, const Key &, Key &>;
    using pointer_type = std::conditional_t<IsConst, const Key *, Key *>;

    constexpr basic_iter() = default;
    constexpr basic_iter(container_pointer s, size_type i) : _s(s), _i(i) {
      advance_to_filled();
    }
    constexpr operator basic_iter<true>() const
      requires(!IsConst)
    {
      basic_iter<true> r;
      r._s = _s;
      r._i = _i;
      return r;
    }

    constexpr reference_type operator*() const { return _s->_slots[_i].key; }
    constexpr pointer_type operator->() const { return &_s->_slots[_i].key; }

    constexpr basic_iter &operator++() {
      ++_i;
      advance_to_filled();
      return *this;
    }
    constexpr basic_iter operator++(int) {
      basic_iter tmp = *this;
      ++(*this);
      return tmp;
    }

    constexpr bool operator==(const basic_iter &o) const {
      return _s == o._s && _i == o._i;
    }
    constexpr bool operator!=(const basic_iter &o) const {
      return !(*this == o);
    }
  };

public:
  using iterator = basic_iter<false>;
  using const_iterator = basic_iter<true>;

  constexpr unordered_set() = default;

  constexpr size_type size() const noexcept { return _size; }
  constexpr bool empty() const noexcept { return _size == 0; }
  constexpr size_type bucket_count() const noexcept { return _slots.size(); }

  constexpr void clear() {
    for (size_type i = 0; i < _slots.size(); ++i)
      _slots[i].state = slot_state::empty;
    _size = 0;
  }

  constexpr iterator begin() noexcept { return iterator(this, 0); }
  constexpr iterator end() noexcept { return iterator(this, _slots.size()); }
  constexpr const_iterator begin() const noexcept {
    return const_iterator(this, 0);
  }
  constexpr const_iterator end() const noexcept {
    return const_iterator(this, _slots.size());
  }

  constexpr iterator find(const Key &k) {
    size_type i = find_slot(k);
    return i == size_type(-1) ? end() : iterator(this, i);
  }
  constexpr const_iterator find(const Key &k) const {
    size_type i = find_slot(k);
    return i == size_type(-1) ? end() : const_iterator(this, i);
  }

  constexpr bool contains(const Key &k) const {
    return find_slot(k) != size_type(-1);
  }

private:
  template <typename K>
  constexpr std::pair<size_type, bool> insert_slot(K &&k) {
    size_type cap = _slots.size();
    size_type h = _hasher(k) % cap;
    size_type first_tombstone = size_type(-1);
    for (size_type probe = 0; probe < cap; ++probe) {
      size_type i = (h + probe) % cap;
      slot &s = _slots[i];
      if (s.state == slot_state::empty) {
        size_type target = (first_tombstone != size_type(-1)) ? first_tombstone
                                                              : i;
        _slots[target].key = std::forward<K>(k);
        _slots[target].state = slot_state::filled;
        ++_size;
        return {target, true};
      }
      if (s.state == slot_state::tombstone) {
        if (first_tombstone == size_type(-1))
          first_tombstone = i;
        continue;
      }
      if (_eq(s.key, k))
        return {i, false};
    }
    return {size_type(-1), false};
  }

public:
  constexpr std::pair<iterator, bool> insert(const Key &k) {
    maybe_grow();
    auto [i, ins] = insert_slot(k);
    return {iterator(this, i), ins};
  }
  constexpr std::pair<iterator, bool> insert(Key &&k) {
    maybe_grow();
    auto [i, ins] = insert_slot(std::move(k));
    return {iterator(this, i), ins};
  }

  template <typename... Args>
  constexpr std::pair<iterator, bool> emplace(Args &&...args) {
    return insert(Key(std::forward<Args>(args)...));
  }

  constexpr size_type erase(const Key &k) {
    size_type i = find_slot(k);
    if (i == size_type(-1))
      return 0;
    _slots[i].state = slot_state::tombstone;
    --_size;
    return 1;
  }
};

} // namespace ycetl
