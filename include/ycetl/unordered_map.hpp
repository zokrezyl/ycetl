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

// Open-addressing hash map. Slot states are empty / filled / tombstone;
// insert linearly probes from the hashed bucket, lookup stops at the
// first empty slot, erase converts to tombstone.
//
// Rehashes by doubling when occupancy crosses 70% of bucket count.
// Storage is a single dynamic_array<slot> default-constructed against
// typed_dynamic_memory<slot> — no external Memory parameter, same shape
// as std::unordered_map.
//
// The previous container::container<>-based skeleton in this file was
// incomplete (referenced an undeclared _alloc_ptr / non-template Memory)
// and never compiled — replaced wholesale.
template <typename Key,
          typename T,
          // Default to ycetl::hash, not std::hash — libstdc++ leaves
          // std::hash<int>::operator() non-constexpr, which would block
          // any compile-time use of the map.
          typename Hash     = hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class unordered_map {
public:
    using key_type    = Key;
    using mapped_type = T;
    // Storage value is plain pair<Key,T> (not pair<const Key,T>) so the
    // slot is move-assignable during rehash. The find/iterator interface
    // surfaces it as the same shape — callers that need
    // std::unordered_map's `const Key` semantics can const_cast at the
    // boundary; in practice nobody mutates .first because the hash would
    // become wrong.
    using value_type  = std::pair<Key, T>;
    using size_type   = std::size_t;
    using hasher      = Hash;
    using key_equal   = KeyEqual;

private:
    enum class slot_state : std::uint8_t { empty = 0, filled = 1, tombstone = 2 };

    struct slot {
        slot_state state = slot_state::empty;
        value_type kv{};
    };

    using storage_type = dynamic_array<slot>;

    static constexpr size_type initial_capacity = 8;
    static constexpr size_type max_load_pct     = 70; // grow at 70% occupancy

    storage_type _slots;
    size_type    _size = 0;
    hasher       _hasher;
    key_equal    _eq;

    constexpr size_type bucket_of(const Key &k) const {
        return _hasher(k) % _slots.size();
    }

    constexpr void ensure_capacity() {
        if (_slots.size() == 0) _slots.resize(initial_capacity);
    }

    // Move the filled entries from _slots into a fresh storage of size
    // new_cap. Tombstones disappear in the process.
    constexpr void rehash(size_type new_cap) {
        storage_type new_slots;
        new_slots.resize(new_cap);
        for (size_type i = 0; i < _slots.size(); ++i) {
            if (_slots[i].state != slot_state::filled) continue;
            size_type h = _hasher(_slots[i].kv.first) % new_cap;
            for (size_type probe = 0; probe < new_cap; ++probe) {
                size_type j = (h + probe) % new_cap;
                if (new_slots[j].state == slot_state::empty) {
                    new_slots[j].kv = std::move(_slots[i].kv);
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

    // Returns the index of the filled slot matching k, or size_type(-1).
    constexpr size_type find_slot(const Key &k) const {
        if (_slots.size() == 0) return size_type(-1);
        size_type h = bucket_of(k);
        size_type cap = _slots.size();
        for (size_type probe = 0; probe < cap; ++probe) {
            size_type i = (h + probe) % cap;
            const slot &s = _slots[i];
            if (s.state == slot_state::empty) return size_type(-1);
            if (s.state == slot_state::filled && _eq(s.kv.first, k)) return i;
            // tombstone → keep probing
        }
        return size_type(-1);
    }

    // The slot iterator skips non-filled slots so a range-for sees only
    // live entries. IsConst is a template flag so iterator and
    // const_iterator share the body.
    template <bool IsConst> class basic_iter {
        friend class unordered_map;
        using container_pointer =
            std::conditional_t<IsConst, const unordered_map *, unordered_map *>;
        container_pointer _m = nullptr;
        size_type _i = 0;

        constexpr void advance_to_filled() {
            while (_m && _i < _m->_slots.size()
                && _m->_slots[_i].state != slot_state::filled)
                ++_i;
        }

    public:
        using value_type      = unordered_map::value_type;
        using reference_type  = std::conditional_t<IsConst,
                                                  const value_type &,
                                                  value_type &>;
        using pointer_type    = std::conditional_t<IsConst,
                                                   const value_type *,
                                                   value_type *>;

        constexpr basic_iter() = default;
        constexpr basic_iter(container_pointer m, size_type i) : _m(m), _i(i) {
            advance_to_filled();
        }
        // non-const → const conversion
        constexpr operator basic_iter<true>() const
            requires(!IsConst)
        {
            basic_iter<true> r;
            r._m = _m;
            r._i = _i;
            return r;
        }

        constexpr reference_type operator*()  const { return _m->_slots[_i].kv; }
        constexpr pointer_type   operator->() const { return &_m->_slots[_i].kv; }

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
            return _m == o._m && _i == o._i;
        }
        constexpr bool operator!=(const basic_iter &o) const {
            return !(*this == o);
        }
    };

public:
    using iterator       = basic_iter<false>;
    using const_iterator = basic_iter<true>;

    constexpr unordered_map() = default;

    constexpr size_type size()  const noexcept { return _size; }
    constexpr bool      empty() const noexcept { return _size == 0; }
    constexpr size_type bucket_count() const noexcept { return _slots.size(); }

    constexpr void clear() {
        for (size_type i = 0; i < _slots.size(); ++i)
            _slots[i].state = slot_state::empty;
        _size = 0;
    }

    constexpr iterator       begin()       noexcept { return iterator(this, 0); }
    constexpr iterator       end()         noexcept { return iterator(this, _slots.size()); }
    constexpr const_iterator begin() const noexcept { return const_iterator(this, 0); }
    constexpr const_iterator end()   const noexcept { return const_iterator(this, _slots.size()); }

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
    // Internal slot-level insert: probes from the hashed bucket; uses
    // the first tombstone seen if no live match exists. Assumes the
    // table has room (caller did maybe_grow() first).
    template <typename V>
    constexpr std::pair<size_type, bool> insert_slot(V &&v) {
        size_type cap = _slots.size();
        size_type h   = _hasher(v.first) % cap;
        size_type first_tombstone = size_type(-1);
        for (size_type probe = 0; probe < cap; ++probe) {
            size_type i = (h + probe) % cap;
            slot &s = _slots[i];
            if (s.state == slot_state::empty) {
                size_type target =
                    (first_tombstone != size_type(-1)) ? first_tombstone : i;
                _slots[target].kv = std::forward<V>(v);
                _slots[target].state = slot_state::filled;
                ++_size;
                return {target, true};
            }
            if (s.state == slot_state::tombstone) {
                if (first_tombstone == size_type(-1)) first_tombstone = i;
                continue;
            }
            if (_eq(s.kv.first, v.first)) return {i, false};
        }
        // Should not happen — rehash keeps us under load factor.
        return {size_type(-1), false};
    }

public:
    constexpr std::pair<iterator, bool> insert(const value_type &v) {
        maybe_grow();
        auto [i, ins] = insert_slot(v);
        return {iterator(this, i), ins};
    }
    constexpr std::pair<iterator, bool> insert(value_type &&v) {
        maybe_grow();
        auto [i, ins] = insert_slot(std::move(v));
        return {iterator(this, i), ins};
    }

    template <typename M>
    constexpr std::pair<iterator, bool> insert_or_assign(const Key &k, M &&v) {
        maybe_grow();
        size_type cap = _slots.size();
        size_type h   = _hasher(k) % cap;
        size_type first_tombstone = size_type(-1);
        for (size_type probe = 0; probe < cap; ++probe) {
            size_type i = (h + probe) % cap;
            slot &s = _slots[i];
            if (s.state == slot_state::empty) {
                size_type target =
                    (first_tombstone != size_type(-1)) ? first_tombstone : i;
                _slots[target].kv = value_type(k, T(std::forward<M>(v)));
                _slots[target].state = slot_state::filled;
                ++_size;
                return {iterator(this, target), true};
            }
            if (s.state == slot_state::tombstone) {
                if (first_tombstone == size_type(-1)) first_tombstone = i;
                continue;
            }
            if (_eq(s.kv.first, k)) {
                s.kv.second = std::forward<M>(v);
                return {iterator(this, i), false};
            }
        }
        return {end(), false};
    }

    constexpr T &operator[](const Key &k) {
        maybe_grow();
        auto [i, ins] = insert_slot(value_type(k, T{}));
        return _slots[i].kv.second;
    }

    constexpr size_type erase(const Key &k) {
        size_type i = find_slot(k);
        if (i == size_type(-1)) return 0;
        _slots[i].state = slot_state::tombstone;
        --_size;
        return 1;
    }
};

} // namespace ycetl
