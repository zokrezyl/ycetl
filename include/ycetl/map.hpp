#pragma once

#include <cstddef>
#include <functional>
#include <utility>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

// Ordered map as a flat sorted dynamic_array of (Key, T) pairs. Lookup
// is O(log n) via binary search; insert / erase are O(n) due to the
// shift. That's the same shape as C++23's std::flat_map and the natural
// choice here:
//
//   - Stays constexpr-clean (a red-black tree's parent pointers don't).
//   - Single contiguous buffer is what the constexpr machine likes
//     anyway — easier to bake into rodata as a value tree.
//   - For the small-to-medium maps (≤ low thousands of keys) that
//     dominate compile-time use, the asymptotics matter less than the
//     cache locality.
//
// Use std::pair<Key, T> rather than std::pair<const Key, T> as the
// storage element so the underlying dynamic_array can move-assign on
// insert/erase. The public-facing value_type still says
// std::pair<const Key, T> to match std::map's interface contract; the
// const_cast happens at the [] / find boundary.
template <typename Key,
          typename T,
          typename Compare = std::less<Key>>
class map {
public:
    using key_type        = Key;
    using mapped_type     = T;
    using value_type      = std::pair<const Key, T>;
    using key_compare     = Compare;
    using size_type       = std::size_t;
    using reference       = value_type &;
    using const_reference = const value_type &;

private:
    using storage_value = std::pair<Key, T>;
    using storage_type  = dynamic_array<storage_value>;
    storage_type  _data;
    key_compare   _cmp;

    // Return the lower-bound index — the first slot whose key is not
    // less than k. Used by both find() (does this key exist?) and
    // insert() (where would a new key go?).
    constexpr size_type lower_bound_index(const Key &k) const {
        size_type lo = 0, hi = _data.size();
        while (lo < hi) {
            size_type mid = lo + (hi - lo) / 2;
            if (_cmp(_data[mid].first, k)) lo = mid + 1;
            else                            hi = mid;
        }
        return lo;
    }

public:
    using iterator       = typename storage_type::iterator;
    using const_iterator = typename storage_type::const_iterator;

    constexpr map() = default;

    template <typename Memory>
    explicit constexpr map(Memory &m) : _data(m), _cmp() {}

    constexpr bool      empty() const noexcept { return _data.size() == 0; }
    constexpr size_type size()  const noexcept { return _data.size(); }

    constexpr void clear() { _data.clear(); }

    constexpr iterator       begin()       noexcept { return _data.begin(); }
    constexpr iterator       end()         noexcept { return _data.end(); }
    constexpr const_iterator begin() const noexcept { return _data.begin(); }
    constexpr const_iterator end()   const noexcept { return _data.end(); }

    constexpr iterator find(const Key &k) {
        size_type i = lower_bound_index(k);
        if (i < _data.size() && !_cmp(k, _data[i].first)) return _data.begin() + i;
        return _data.end();
    }
    constexpr const_iterator find(const Key &k) const {
        size_type i = lower_bound_index(k);
        if (i < _data.size() && !_cmp(k, _data[i].first)) return _data.begin() + i;
        return _data.end();
    }

    constexpr bool contains(const Key &k) const { return find(k) != end(); }

    // {iterator, inserted?} — false when the key already exists.
    constexpr std::pair<iterator, bool> insert(const value_type &v) {
        size_type i = lower_bound_index(v.first);
        if (i < _data.size() && !_cmp(v.first, _data[i].first))
            return {_data.begin() + i, false};
        _data.insert(_data.begin() + i, storage_value(v.first, v.second));
        return {_data.begin() + i, true};
    }
    constexpr std::pair<iterator, bool> insert(value_type &&v) {
        size_type i = lower_bound_index(v.first);
        if (i < _data.size() && !_cmp(v.first, _data[i].first))
            return {_data.begin() + i, false};
        _data.insert(_data.begin() + i,
                     storage_value(std::move(v.first), std::move(v.second)));
        return {_data.begin() + i, true};
    }

    template <typename M>
    constexpr std::pair<iterator, bool> insert_or_assign(const Key &k, M &&v) {
        size_type i = lower_bound_index(k);
        if (i < _data.size() && !_cmp(k, _data[i].first)) {
            _data[i].second = std::forward<M>(v);
            return {_data.begin() + i, false};
        }
        _data.insert(_data.begin() + i, storage_value(k, T(std::forward<M>(v))));
        return {_data.begin() + i, true};
    }

    constexpr T &operator[](const Key &k) {
        size_type i = lower_bound_index(k);
        if (i < _data.size() && !_cmp(k, _data[i].first))
            return _data[i].second;
        _data.insert(_data.begin() + i, storage_value(k, T{}));
        return _data[i].second;
    }

    constexpr T &at(const Key &k) {
        size_type i = lower_bound_index(k);
        return _data[i].second;
    }
    constexpr const T &at(const Key &k) const {
        size_type i = lower_bound_index(k);
        return _data[i].second;
    }

    // Order-preserving erase — shift instead of swap-and-pop, since
    // ordering is the map's invariant.
    constexpr size_type erase(const Key &k) {
        size_type i = lower_bound_index(k);
        if (i >= _data.size() || _cmp(k, _data[i].first)) return 0;
        size_type n = _data.size();
        for (size_type j = i; j + 1 < n; ++j)
            _data[j] = std::move(_data[j + 1]);
        _data.pop_back();
        return 1;
    }
};

} // namespace ycetl
