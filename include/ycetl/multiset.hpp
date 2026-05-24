#pragma once

#include <cstddef>
#include <functional>
#include <utility>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

// Flat sorted multiset — same storage shape as ycetl::set, but every
// insert succeeds and find/count/erase respect equal-key runs.
//
// Equal-key runs are stored contiguously. Insertion uses upper_bound so
// new equal keys go AFTER existing ones (FIFO order within a key) —
// matches std::multiset.
template <typename Key, typename Compare = std::less<Key>>
class multiset {
public:
    using key_type    = Key;
    using value_type  = Key;
    using key_compare = Compare;
    using size_type   = std::size_t;

private:
    using storage_type = dynamic_array<Key>;
    storage_type _data;
    key_compare  _cmp;

    constexpr size_type lower_bound_index(const Key &k) const {
        size_type lo = 0, hi = _data.size();
        while (lo < hi) {
            size_type mid = lo + (hi - lo) / 2;
            if (_cmp(_data[mid], k)) lo = mid + 1;
            else                      hi = mid;
        }
        return lo;
    }
    constexpr size_type upper_bound_index(const Key &k) const {
        size_type lo = 0, hi = _data.size();
        while (lo < hi) {
            size_type mid = lo + (hi - lo) / 2;
            if (!_cmp(k, _data[mid])) lo = mid + 1;
            else                       hi = mid;
        }
        return lo;
    }

public:
    using iterator       = typename storage_type::iterator;
    using const_iterator = typename storage_type::const_iterator;

    constexpr multiset() = default;
    template <typename Memory> explicit constexpr multiset(Memory &m) : _data(m), _cmp() {}

    constexpr bool      empty() const noexcept { return _data.size() == 0; }
    constexpr size_type size()  const noexcept { return _data.size(); }
    constexpr void      clear()                 { _data.clear(); }

    constexpr iterator       begin()       noexcept { return _data.begin(); }
    constexpr iterator       end()         noexcept { return _data.end(); }
    constexpr const_iterator begin() const noexcept { return _data.begin(); }
    constexpr const_iterator end()   const noexcept { return _data.end(); }

    constexpr iterator find(const Key &k) {
        size_type i = lower_bound_index(k);
        if (i < _data.size() && !_cmp(k, _data[i])) return _data.begin() + i;
        return _data.end();
    }
    constexpr const_iterator find(const Key &k) const {
        size_type i = lower_bound_index(k);
        if (i < _data.size() && !_cmp(k, _data[i])) return _data.begin() + i;
        return _data.end();
    }
    constexpr bool contains(const Key &k) const { return find(k) != end(); }

    constexpr size_type count(const Key &k) const {
        return upper_bound_index(k) - lower_bound_index(k);
    }
    constexpr std::pair<iterator, iterator> equal_range(const Key &k) {
        size_type lo = lower_bound_index(k);
        size_type hi = upper_bound_index(k);
        return {_data.begin() + lo, _data.begin() + hi};
    }

    constexpr iterator insert(const Key &k) {
        size_type i = upper_bound_index(k);
        _data.insert(_data.begin() + i, k);
        return _data.begin() + i;
    }
    constexpr iterator insert(Key &&k) {
        size_type i = upper_bound_index(k);
        _data.insert(_data.begin() + i, std::move(k));
        return _data.begin() + i;
    }

    template <typename... Args>
    constexpr iterator emplace(Args &&...args) {
        return insert(Key(std::forward<Args>(args)...));
    }

    // Remove every element matching k. Returns the count removed.
    constexpr size_type erase(const Key &k) {
        size_type lo = lower_bound_index(k);
        size_type hi = upper_bound_index(k);
        if (lo == hi) return 0;
        size_type removed = hi - lo;
        size_type n = _data.size();
        for (size_type j = lo; j + removed < n; ++j)
            _data[j] = std::move(_data[j + removed]);
        for (size_type j = 0; j < removed; ++j) _data.pop_back();
        return removed;
    }
};

} // namespace ycetl
