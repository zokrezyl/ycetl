#pragma once

#include <cstddef>
#include <functional>
#include <utility>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

// Ordered set as a flat sorted dynamic_array. Same shape decisions as
// ycetl::map: binary-search lookup, ordered insert, order-preserving
// erase. Use this — not the broken container::container<>-based earlier
// design — for actual ordered set semantics.
//
// (The old set.hpp was a template against the unimplemented
// container::container_traits<set, type_set<...>>::default_memory glue
// that never compiled; replaced wholesale.)
template <typename Key, typename Compare = std::less<Key>> class set {
public:
  using key_type = Key;
  using value_type = Key;
  using key_compare = Compare;
  using size_type = std::size_t;

private:
  using storage_type = dynamic_array<Key>;
  storage_type _data;
  key_compare _cmp;

  constexpr size_type lower_bound_index(const Key &k) const {
    size_type lo = 0, hi = _data.size();
    while (lo < hi) {
      size_type mid = lo + (hi - lo) / 2;
      if (_cmp(_data[mid], k))
        lo = mid + 1;
      else
        hi = mid;
    }
    return lo;
  }

public:
  using iterator = typename storage_type::iterator;
  using const_iterator = typename storage_type::const_iterator;

  constexpr set() = default;

  template <typename Memory>
  explicit constexpr set(Memory &m) : _data(m), _cmp() {}

  constexpr bool empty() const noexcept { return _data.size() == 0; }
  constexpr size_type size() const noexcept { return _data.size(); }

  constexpr void clear() { _data.clear(); }

  constexpr iterator begin() noexcept { return _data.begin(); }
  constexpr iterator end() noexcept { return _data.end(); }
  constexpr const_iterator begin() const noexcept { return _data.begin(); }
  constexpr const_iterator end() const noexcept { return _data.end(); }

  constexpr iterator find(const Key &k) {
    size_type i = lower_bound_index(k);
    if (i < _data.size() && !_cmp(k, _data[i]))
      return _data.begin() + i;
    return _data.end();
  }
  constexpr const_iterator find(const Key &k) const {
    size_type i = lower_bound_index(k);
    if (i < _data.size() && !_cmp(k, _data[i]))
      return _data.begin() + i;
    return _data.end();
  }

  constexpr bool contains(const Key &k) const { return find(k) != end(); }

  constexpr std::pair<iterator, bool> insert(const Key &k) {
    size_type i = lower_bound_index(k);
    if (i < _data.size() && !_cmp(k, _data[i]))
      return {_data.begin() + i, false};
    _data.insert(_data.begin() + i, k);
    return {_data.begin() + i, true};
  }
  constexpr std::pair<iterator, bool> insert(Key &&k) {
    size_type i = lower_bound_index(k);
    if (i < _data.size() && !_cmp(k, _data[i]))
      return {_data.begin() + i, false};
    _data.insert(_data.begin() + i, std::move(k));
    return {_data.begin() + i, true};
  }

  template <typename... Args>
  constexpr std::pair<iterator, bool> emplace(Args &&...args) {
    return insert(Key(std::forward<Args>(args)...));
  }

  constexpr size_type erase(const Key &k) {
    size_type i = lower_bound_index(k);
    if (i >= _data.size() || _cmp(k, _data[i]))
      return 0;
    size_type n = _data.size();
    for (size_type j = i; j + 1 < n; ++j)
      _data[j] = std::move(_data[j + 1]);
    _data.pop_back();
    return 1;
  }
};

} // namespace ycetl
