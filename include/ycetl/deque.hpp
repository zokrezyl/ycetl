#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

// Ring-buffer deque. Single contiguous storage with head/tail indices
// that wrap; doubles in size when full. O(1) push_front / push_back /
// pop_front / pop_back / operator[]; O(n) insert / erase in the middle
// (which the standard doesn't even require — std::deque is allowed to
// invalidate iterators on those too).
//
// This is intentionally NOT the libstdc++ block-of-blocks design. That
// shape is great for the property "iterators stay valid across
// push_back", which the standard demands. The ring buffer gives that
// up — any growth invalidates everything, same as ycetl::vector —
// in exchange for a single allocation, friendly to constexpr
// evaluation, and trivially baked into a baked-in struct via the same
// pattern as the other ycetl sequences. If you need the
// iterator-stability property, use ycetl::list.
//
// Replaces the previous empty deque stub.
template <typename T, typename TypedMemory = typed_dynamic_memory<T>>
class deque {
public:
    using storage_type    = dynamic_array<T, TypedMemory>;
    using value_type      = T;
    using size_type       = std::size_t;
    using reference       = T &;
    using const_reference = const T &;

private:
    static constexpr size_type initial_capacity = 8;

    storage_type _buf;   // sized to the ring capacity, never grown via push_back
    size_type    _head = 0;   // index of front()
    size_type    _size = 0;

    constexpr size_type cap() const { return _buf.size(); }

    constexpr size_type tail_index() const {
        return cap() == 0 ? 0 : (_head + _size) % cap();
    }

    // Grow to new_cap and re-lay the elements contiguously starting at
    // 0. The ring becomes linear again — head wraps no more after a
    // grow.
    constexpr void grow(size_type new_cap) {
        storage_type fresh;
        fresh.resize(new_cap);
        for (size_type i = 0; i < _size; ++i)
            fresh[i] = std::move(_buf[(_head + i) % cap()]);
        _buf  = std::move(fresh);
        _head = 0;
    }

    constexpr void ensure_room() {
        if (_size + 1 > cap()) grow(cap() == 0 ? initial_capacity : cap() * 2);
    }

public:
    constexpr deque() = default;

    constexpr size_type size()  const noexcept { return _size; }
    constexpr bool      empty() const noexcept { return _size == 0; }

    constexpr void clear() {
        _buf.clear();
        _head = 0;
        _size = 0;
    }

    constexpr reference       operator[](size_type i)       { return _buf[(_head + i) % cap()]; }
    constexpr const_reference operator[](size_type i) const { return _buf[(_head + i) % cap()]; }

    constexpr reference       front()       { return _buf[_head]; }
    constexpr const_reference front() const { return _buf[_head]; }
    constexpr reference       back()        { return _buf[(_head + _size - 1) % cap()]; }
    constexpr const_reference back()  const { return _buf[(_head + _size - 1) % cap()]; }

    constexpr void push_back(const T &v) {
        ensure_room();
        _buf[tail_index()] = v;
        ++_size;
    }
    constexpr void push_back(T &&v) {
        ensure_room();
        _buf[tail_index()] = std::move(v);
        ++_size;
    }

    constexpr void push_front(const T &v) {
        ensure_room();
        _head = (_head + cap() - 1) % cap();
        _buf[_head] = v;
        ++_size;
    }
    constexpr void push_front(T &&v) {
        ensure_room();
        _head = (_head + cap() - 1) % cap();
        _buf[_head] = std::move(v);
        ++_size;
    }

    constexpr void pop_front() {
        // Leave the slot's value as-is — caller has already had their
        // shot at it via front(). Move head forward and decrement.
        _head = (_head + 1) % cap();
        --_size;
    }
    constexpr void pop_back() { --_size; }

    // Range-for support over a deque iterates from front() to back().
    template <bool IsConst> class basic_iter {
        friend class deque;
        using container_pointer =
            std::conditional_t<IsConst, const deque *, deque *>;
        container_pointer _d   = nullptr;
        size_type _i = 0;   // logical index from front()

    public:
        using value_type      = T;
        using reference_type  = std::conditional_t<IsConst, const T &, T &>;
        using pointer_type    = std::conditional_t<IsConst, const T *, T *>;

        constexpr basic_iter() = default;
        constexpr basic_iter(container_pointer d, size_type i) : _d(d), _i(i) {}

        constexpr operator basic_iter<true>() const
            requires(!IsConst) { basic_iter<true> r; r._d = _d; r._i = _i; return r; }

        constexpr reference_type operator*()  const { return (*_d)[_i]; }
        constexpr pointer_type   operator->() const { return &(*_d)[_i]; }

        constexpr basic_iter &operator++() { ++_i; return *this; }
        constexpr basic_iter  operator++(int) { auto t = *this; ++(*this); return t; }
        constexpr basic_iter &operator--() { --_i; return *this; }
        constexpr basic_iter  operator--(int) { auto t = *this; --(*this); return t; }

        constexpr bool operator==(const basic_iter &o) const { return _d == o._d && _i == o._i; }
        constexpr bool operator!=(const basic_iter &o) const { return !(*this == o); }
    };

    using iterator       = basic_iter<false>;
    using const_iterator = basic_iter<true>;

    constexpr iterator       begin()       noexcept { return iterator(this, 0); }
    constexpr iterator       end()         noexcept { return iterator(this, _size); }
    constexpr const_iterator begin() const noexcept { return const_iterator(this, 0); }
    constexpr const_iterator end()   const noexcept { return const_iterator(this, _size); }
};

} // namespace ycetl
