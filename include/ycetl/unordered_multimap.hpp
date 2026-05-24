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

// Open-addressing hash multimap. Same shape as unordered_multiset, but
// each slot carries a (Key, T) pair.
template <typename Key,
          typename T,
          typename Hash     = hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class unordered_multimap {
public:
    using key_type    = Key;
    using mapped_type = T;
    using value_type  = std::pair<Key, T>;
    using size_type   = std::size_t;
    using hasher      = Hash;
    using key_equal   = KeyEqual;

private:
    enum class slot_state : std::uint8_t { empty = 0, filled = 1, tombstone = 2 };
    struct slot { slot_state state = slot_state::empty; value_type kv{}; };

    using storage_type = dynamic_array<slot>;
    static constexpr size_type initial_capacity = 8;
    static constexpr size_type max_load_pct     = 70;

    storage_type _slots;
    size_type    _size = 0;
    hasher       _hasher;
    key_equal    _eq;

    constexpr void rehash(size_type new_cap) {
        storage_type ns;
        ns.resize(new_cap);
        for (size_type i = 0; i < _slots.size(); ++i) {
            if (_slots[i].state != slot_state::filled) continue;
            size_type h = _hasher(_slots[i].kv.first) % new_cap;
            for (size_type p = 0; p < new_cap; ++p) {
                size_type j = (h + p) % new_cap;
                if (ns[j].state == slot_state::empty) {
                    ns[j].kv    = std::move(_slots[i].kv);
                    ns[j].state = slot_state::filled;
                    break;
                }
            }
        }
        _slots = std::move(ns);
    }

    constexpr void maybe_grow() {
        if (_slots.size() == 0) { _slots.resize(initial_capacity); return; }
        if ((_size + 1) * 100 > _slots.size() * max_load_pct)
            rehash(_slots.size() * 2);
    }

    template <bool IsConst> class basic_iter {
        friend class unordered_multimap;
        using container_pointer =
            std::conditional_t<IsConst, const unordered_multimap *, unordered_multimap *>;
        container_pointer _m = nullptr;
        size_type _i = 0;
        constexpr void advance_to_filled() {
            while (_m && _i < _m->_slots.size()
                && _m->_slots[_i].state != slot_state::filled) ++_i;
        }
    public:
        using value_type     = unordered_multimap::value_type;
        using reference_type = std::conditional_t<IsConst, const value_type &, value_type &>;
        using pointer_type   = std::conditional_t<IsConst, const value_type *, value_type *>;

        constexpr basic_iter() = default;
        constexpr basic_iter(container_pointer m, size_type i) : _m(m), _i(i) {
            advance_to_filled();
        }
        constexpr operator basic_iter<true>() const requires(!IsConst) {
            basic_iter<true> r; r._m = _m; r._i = _i; return r;
        }
        constexpr reference_type operator*()  const { return _m->_slots[_i].kv; }
        constexpr pointer_type   operator->() const { return &_m->_slots[_i].kv; }
        constexpr basic_iter &operator++() { ++_i; advance_to_filled(); return *this; }
        constexpr basic_iter operator++(int) { auto t = *this; ++(*this); return t; }
        constexpr bool operator==(const basic_iter &o) const { return _m == o._m && _i == o._i; }
        constexpr bool operator!=(const basic_iter &o) const { return !(*this == o); }
    };

public:
    using iterator       = basic_iter<false>;
    using const_iterator = basic_iter<true>;

    constexpr unordered_multimap() = default;

    constexpr size_type size()  const noexcept { return _size; }
    constexpr bool      empty() const noexcept { return _size == 0; }
    constexpr size_type bucket_count() const noexcept { return _slots.size(); }
    constexpr void clear() {
        for (size_type i = 0; i < _slots.size(); ++i) _slots[i].state = slot_state::empty;
        _size = 0;
    }

    constexpr iterator       begin()       noexcept { return iterator(this, 0); }
    constexpr iterator       end()         noexcept { return iterator(this, _slots.size()); }
    constexpr const_iterator begin() const noexcept { return const_iterator(this, 0); }
    constexpr const_iterator end()   const noexcept { return const_iterator(this, _slots.size()); }

    constexpr iterator find(const Key &k) {
        if (_slots.size() == 0) return end();
        size_type cap = _slots.size(), h = _hasher(k) % cap;
        for (size_type p = 0; p < cap; ++p) {
            size_type i = (h + p) % cap;
            if (_slots[i].state == slot_state::empty) return end();
            if (_slots[i].state == slot_state::filled && _eq(_slots[i].kv.first, k))
                return iterator(this, i);
        }
        return end();
    }

    constexpr bool contains(const Key &k) const {
        if (_slots.size() == 0) return false;
        size_type cap = _slots.size(), h = _hasher(k) % cap;
        for (size_type p = 0; p < cap; ++p) {
            size_type i = (h + p) % cap;
            if (_slots[i].state == slot_state::empty) return false;
            if (_slots[i].state == slot_state::filled && _eq(_slots[i].kv.first, k))
                return true;
        }
        return false;
    }

    constexpr size_type count(const Key &k) const {
        if (_slots.size() == 0) return 0;
        size_type cap = _slots.size(), h = _hasher(k) % cap, c = 0;
        for (size_type p = 0; p < cap; ++p) {
            size_type i = (h + p) % cap;
            if (_slots[i].state == slot_state::empty) return c;
            if (_slots[i].state == slot_state::filled && _eq(_slots[i].kv.first, k))
                ++c;
        }
        return c;
    }

private:
    template <typename V>
    constexpr iterator insert_impl(V &&v) {
        maybe_grow();
        size_type cap = _slots.size(), h = _hasher(v.first) % cap;
        for (size_type p = 0; p < cap; ++p) {
            size_type i = (h + p) % cap;
            if (_slots[i].state != slot_state::filled) {
                _slots[i].kv    = std::forward<V>(v);
                _slots[i].state = slot_state::filled;
                ++_size;
                return iterator(this, i);
            }
        }
        return end();
    }

public:
    constexpr iterator insert(const value_type &v) { return insert_impl(v); }
    constexpr iterator insert(value_type &&v)      { return insert_impl(std::move(v)); }

    constexpr size_type erase(const Key &k) {
        if (_slots.size() == 0) return 0;
        size_type cap = _slots.size(), h = _hasher(k) % cap, removed = 0;
        for (size_type p = 0; p < cap; ++p) {
            size_type i = (h + p) % cap;
            if (_slots[i].state == slot_state::empty) break;
            if (_slots[i].state == slot_state::filled && _eq(_slots[i].kv.first, k)) {
                _slots[i].state = slot_state::tombstone;
                ++removed;
            }
        }
        _size -= removed;
        return removed;
    }
};

} // namespace ycetl
