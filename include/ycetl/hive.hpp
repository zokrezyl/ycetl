#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

// Reduced ycetl-flavoured take on std::hive (C++26 P0447): a sequence
// container with O(1) insert and O(1) erase-by-iterator, where erased
// slots are recycled by the next insert.
//
// The standard's hive is a list of fixed-size blocks with per-element
// skipfields, picked specifically so insert/erase NEVER invalidate
// other iterators. We use a single growable storage with a free-list
// instead — same usage model (insert anywhere, erase by iterator,
// iterate skipping holes) and the same O(1) bounds, with one honest
// caveat: a hive growth (when all slots are filled AND the free-list
// is empty) DOES invalidate iterators, same as ycetl::vector.
// Iterators stay stable across insert into the free-list pool and
// across erase. If you need bulletproof "iterators never invalidate"
// semantics, reserve() ahead of time.
template <typename T> class hive {
private:
    enum class slot_state : std::uint8_t { empty = 0, filled = 1 };

    struct slot {
        slot_state state = slot_state::empty;
        T          value{};
    };

    dynamic_array<slot>        _slots;
    dynamic_array<std::size_t> _free;   // stack of indices of empty slots
    std::size_t                _size = 0;

public:
    using value_type = T;
    using size_type  = std::size_t;

    template <bool IsConst> class basic_iter {
        friend class hive;
        using container_pointer =
            std::conditional_t<IsConst, const hive *, hive *>;
        container_pointer _h = nullptr;
        size_type _i = 0;

        constexpr void advance_to_filled() {
            while (_h && _i < _h->_slots.size()
                && _h->_slots[_i].state != slot_state::filled)
                ++_i;
        }

    public:
        using value_type     = T;
        using reference_type = std::conditional_t<IsConst, const T &, T &>;
        using pointer_type   = std::conditional_t<IsConst, const T *, T *>;

        constexpr basic_iter() = default;
        constexpr basic_iter(container_pointer h, size_type i) : _h(h), _i(i) {
            advance_to_filled();
        }
        constexpr operator basic_iter<true>() const
            requires(!IsConst) { basic_iter<true> r; r._h = _h; r._i = _i; return r; }

        constexpr reference_type operator*()  const { return _h->_slots[_i].value; }
        constexpr pointer_type   operator->() const { return &_h->_slots[_i].value; }

        constexpr basic_iter &operator++() { ++_i; advance_to_filled(); return *this; }
        constexpr basic_iter  operator++(int) { auto t = *this; ++(*this); return t; }

        constexpr bool operator==(const basic_iter &o) const { return _h == o._h && _i == o._i; }
        constexpr bool operator!=(const basic_iter &o) const { return !(*this == o); }
    };

    using iterator       = basic_iter<false>;
    using const_iterator = basic_iter<true>;

    constexpr hive() = default;

    constexpr size_type size()     const noexcept { return _size; }
    constexpr bool      empty()    const noexcept { return _size == 0; }
    constexpr size_type capacity() const noexcept { return _slots.size(); }

    constexpr void reserve(size_type n) {
        if (n > _slots.size()) {
            size_type old = _slots.size();
            _slots.resize(n);
            // Newly-created slots are empty — make them available to
            // the free list so the next inserts land there in O(1).
            for (size_type i = old; i < n; ++i) _free.push_back(i);
        }
    }

    constexpr void clear() {
        _slots.clear();
        _free.clear();
        _size = 0;
    }

    constexpr iterator       begin()       noexcept { return iterator(this, 0); }
    constexpr iterator       end()         noexcept { return iterator(this, _slots.size()); }
    constexpr const_iterator begin() const noexcept { return const_iterator(this, 0); }
    constexpr const_iterator end()   const noexcept { return const_iterator(this, _slots.size()); }

    // O(1) insert. If the free-list has a recycled slot, fill it
    // there; otherwise push a new slot at the end.
    constexpr iterator insert(const T &v) {
        size_type idx;
        if (_free.size() > 0) {
            idx = _free[_free.size() - 1];
            _free.pop_back();
            _slots[idx].value = v;
            _slots[idx].state = slot_state::filled;
        } else {
            slot s;
            s.state = slot_state::filled;
            s.value = v;
            _slots.push_back(std::move(s));
            idx = _slots.size() - 1;
        }
        ++_size;
        return iterator(this, idx);
    }
    constexpr iterator insert(T &&v) {
        size_type idx;
        if (_free.size() > 0) {
            idx = _free[_free.size() - 1];
            _free.pop_back();
            _slots[idx].value = std::move(v);
            _slots[idx].state = slot_state::filled;
        } else {
            slot s;
            s.state = slot_state::filled;
            s.value = std::move(v);
            _slots.push_back(std::move(s));
            idx = _slots.size() - 1;
        }
        ++_size;
        return iterator(this, idx);
    }

    template <typename... Args>
    constexpr iterator emplace(Args &&...args) {
        return insert(T(std::forward<Args>(args)...));
    }

    // O(1) erase. The slot is marked empty and pushed to the free
    // list; every OTHER iterator stays valid.
    constexpr iterator erase(iterator pos) {
        size_type idx = pos._i;
        _slots[idx].state = slot_state::empty;
        _free.push_back(idx);
        --_size;
        // Return an iterator to the next live slot.
        return iterator(this, idx + 1);
    }
};

} // namespace ycetl
