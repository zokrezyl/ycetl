#pragma once

#include <cstddef>
#include <functional>
#include <utility>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

// Flat unordered_set: dynamic_array<Key> + linear scan. O(n) lookup
// instead of O(1) — a deliberate trade for staying constexpr-clean and
// allocation-cheap for the small-N (≤ low hundreds) sets these
// containers see at compile time. The interface is the relevant subset
// of std::unordered_set, so swapping in a real hash-bucket impl later
// is mechanical.
//
// Hash is unused today (kept in the signature so callers can pass
// std::hash<K> without churn when the real bucket impl lands).
template <typename Key,
          typename Hash     = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Storage  = dynamic_array<Key>>
class unordered_set {
public:
    using key_type        = Key;
    using value_type      = Key;
    using size_type       = std::size_t;
    using hasher          = Hash;
    using key_equal       = KeyEqual;
    using storage_type    = Storage;
    using iterator        = typename Storage::iterator;
    using const_iterator  = typename Storage::const_iterator;

private:
    storage_type _data;
    key_equal    _eq;

public:
    constexpr unordered_set() = default;

    template <typename Memory>
    explicit constexpr unordered_set(Memory &m) : _data(m), _eq() {}

    constexpr bool      empty() const noexcept { return _data.size() == 0; }
    constexpr size_type size()  const noexcept { return _data.size(); }

    constexpr void clear() { _data.clear(); }

    constexpr iterator       begin()       noexcept { return _data.begin(); }
    constexpr iterator       end()         noexcept { return _data.end(); }
    constexpr const_iterator begin() const noexcept { return _data.begin(); }
    constexpr const_iterator end()   const noexcept { return _data.end(); }

    constexpr iterator find(const Key &k) {
        for (auto it = _data.begin(); it != _data.end(); ++it)
            if (_eq(*it, k)) return it;
        return _data.end();
    }
    constexpr const_iterator find(const Key &k) const {
        for (auto it = _data.begin(); it != _data.end(); ++it)
            if (_eq(*it, k)) return it;
        return _data.end();
    }

    constexpr bool contains(const Key &k) const { return find(k) != end(); }

    constexpr std::pair<iterator, bool> insert(const Key &k) {
        auto it = find(k);
        if (it != _data.end()) return {it, false};
        _data.push_back(k);
        return {_data.end() - 1, true};
    }
    constexpr std::pair<iterator, bool> insert(Key &&k) {
        auto it = find(k);
        if (it != _data.end()) return {it, false};
        _data.push_back(std::move(k));
        return {_data.end() - 1, true};
    }

    template <typename... Args>
    constexpr std::pair<iterator, bool> emplace(Args &&...args) {
        return insert(Key(std::forward<Args>(args)...));
    }

    // Order is not preserved on erase — swap-and-pop. Fine for an
    // unordered_set, and sidesteps the dynamic_array iterator→pointer
    // gap (erase(pointer) is the only signature; .iterator's _ptr is
    // private).
    constexpr size_type erase(const Key &k) {
        size_type n = _data.size();
        for (size_type i = 0; i < n; ++i) {
            if (_eq(_data[i], k)) {
                if (i + 1 < n)
                    _data[i] = std::move(_data[n - 1]);
                _data.pop_back();
                return 1;
            }
        }
        return 0;
    }
};

} // namespace ycetl
