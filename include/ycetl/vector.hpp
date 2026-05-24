#pragma once

#include <cstddef>
#include <initializer_list>
#include <utility>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

// ycetl::vector is a thin wrapper around ycetl::dynamic_array — the
// proven sequence backbone every other container in this library is
// built on. It exists so `#include <ycetl/vector.hpp>` does the obvious
// thing, and to surface a few std::vector-shaped niceties
// (front/back/data() const, equality, default-constructible without an
// explicit Memory argument) that dynamic_array doesn't expose directly.
//
// The previous vector.hpp template-on-container_traits design never
// compiled in tree; replaced wholesale.
template <typename T, typename TypedMemory = typed_dynamic_memory<T>>
class vector {
public:
    using storage_type    = dynamic_array<T, TypedMemory>;
    using value_type      = T;
    using size_type       = typename storage_type::size_type;
    using difference_type = typename storage_type::difference_type;
    using reference       = T &;
    using const_reference = const T &;
    using iterator        = typename storage_type::iterator;
    using const_iterator  = typename storage_type::const_iterator;

private:
    storage_type _data;

public:
    constexpr vector() = default;

    template <typename Memory>
    explicit constexpr vector(Memory &m) : _data(m) {}

    template <typename Memory>
    constexpr vector(Memory &m, size_type n) : _data(m, n) {}

    template <typename Memory>
    constexpr vector(Memory &m, size_type n, const T &v) : _data(m, n, v) {}

    template <typename Memory>
    constexpr vector(Memory &m, std::initializer_list<T> il) : _data(m, il) {}

    // size / capacity
    constexpr size_type size()     const noexcept { return _data.size(); }
    constexpr size_type capacity() const noexcept { return _data.capacity(); }
    constexpr bool      empty()    const noexcept { return _data.size() == 0; }

    constexpr void reserve(size_type n)              { _data.reserve(n); }
    constexpr void resize(size_type n)               { _data.resize(n); }
    constexpr void resize(size_type n, const T &v)   { _data.resize(n, v); }
    constexpr void clear()                            { _data.clear(); }

    // element access
    constexpr reference       operator[](size_type i)       { return _data[i]; }
    constexpr const_reference operator[](size_type i) const { return _data[i]; }
    constexpr reference       front()       { return _data[0]; }
    constexpr const_reference front() const { return _data[0]; }
    constexpr reference       back()        { return _data[_data.size() - 1]; }
    constexpr const_reference back()  const { return _data[_data.size() - 1]; }

    constexpr auto data()       noexcept { return _data.begin(); }
    constexpr auto data() const noexcept { return _data.begin(); }

    // modifiers
    constexpr void push_back(const T &v) { _data.push_back(v); }
    constexpr void push_back(T &&v)      { _data.push_back(std::move(v)); }

    template <typename... Args>
    constexpr reference emplace_back(Args &&...args) {
        return _data.emplace_back(std::forward<Args>(args)...);
    }

    constexpr void pop_back() { _data.pop_back(); }

    constexpr iterator insert(iterator pos, const T &v) { return _data.insert(pos, v); }
    constexpr iterator insert(iterator pos, T &&v)      { return _data.insert(pos, std::move(v)); }

    // iterators
    constexpr iterator       begin()       noexcept { return _data.begin(); }
    constexpr iterator       end()         noexcept { return _data.end(); }
    constexpr const_iterator begin() const noexcept { return _data.begin(); }
    constexpr const_iterator end()   const noexcept { return _data.end(); }

    // direct storage handle for callers that want the dynamic_array API
    constexpr storage_type       &storage()       noexcept { return _data; }
    constexpr const storage_type &storage() const noexcept { return _data; }

    constexpr bool operator==(const vector &o) const {
        if (_data.size() != o._data.size()) return false;
        for (size_type i = 0; i < _data.size(); ++i)
            if (_data[i] != o._data[i]) return false;
        return true;
    }
};

} // namespace ycetl
