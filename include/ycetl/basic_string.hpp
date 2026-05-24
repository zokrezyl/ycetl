#pragma once

#include <cstddef>
#include <initializer_list>
#include <string_view>
#include <utility>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

// basic_string<CharT>: thin string-shaped wrapper around dynamic_array.
// Stores the characters contiguously (no null terminator inside the
// container — c_str() returns a std::string_view, not a const CharT*,
// to keep the type system honest about there being no trailing NUL).
//
// Replaces the previous container::container<>-based skeleton, which
// never compiled in tree. Char-traits parameter is in the signature
// for std::basic_string-shaped genericity, but every comparison /
// search uses the language's built-in operator==/!= so the typical
// std::char_traits<char> default is invisible to users.
template <typename CharT,
          typename Traits     = void,           // kept for API parity, unused
          typename TypedMemory = typed_dynamic_memory<CharT>>
class basic_string {
public:
    using storage_type = dynamic_array<CharT, TypedMemory>;
    using value_type   = CharT;
    using size_type    = typename storage_type::size_type;
    using iterator     = typename storage_type::iterator;
    using const_iterator = typename storage_type::const_iterator;

private:
    storage_type _data;

    constexpr void append_raw(const CharT *s, size_type n) {
        for (size_type i = 0; i < n; ++i) _data.push_back(s[i]);
    }

    // strlen, constexpr-safe.
    static constexpr size_type len_of(const CharT *s) {
        size_type n = 0;
        while (s[n] != CharT{}) ++n;
        return n;
    }

public:
    static constexpr size_type npos = static_cast<size_type>(-1);

    constexpr basic_string() = default;

    template <typename Memory>
    explicit constexpr basic_string(Memory &m) : _data(m) {}

    template <typename Memory>
    constexpr basic_string(Memory &m, const CharT *s)
        : _data(m) { append_raw(s, len_of(s)); }

    template <typename Memory>
    constexpr basic_string(Memory &m, std::basic_string_view<CharT> sv)
        : _data(m) { append_raw(sv.data(), sv.size()); }

    template <typename Memory>
    constexpr basic_string(Memory &m, std::initializer_list<CharT> il)
        : _data(m, il) {}

    // Default-memory convenience constructors (use the per-T backend
    // default, no explicit Memory argument).
    constexpr basic_string(const CharT *s)               { append_raw(s, len_of(s)); }
    constexpr basic_string(std::basic_string_view<CharT> sv) { append_raw(sv.data(), sv.size()); }

    // size / capacity
    constexpr size_type size()     const noexcept { return _data.size(); }
    constexpr size_type length()   const noexcept { return _data.size(); }
    constexpr size_type capacity() const noexcept { return _data.capacity(); }
    constexpr bool      empty()    const noexcept { return _data.size() == 0; }

    constexpr void reserve(size_type n) { _data.reserve(n); }
    constexpr void clear()              { _data.clear(); }

    // element access
    constexpr CharT       &operator[](size_type i)       { return _data[i]; }
    constexpr const CharT &operator[](size_type i) const { return _data[i]; }
    constexpr CharT       &front()                       { return _data[0]; }
    constexpr const CharT &front()                 const { return _data[0]; }
    constexpr CharT       &back()                        { return _data[_data.size() - 1]; }
    constexpr const CharT &back()                  const { return _data[_data.size() - 1]; }

    // No trailing-NUL contract — return a string_view so callers can't
    // accidentally pass us to a C API expecting null-termination.
    constexpr std::basic_string_view<CharT> view() const noexcept {
        if (_data.size() == 0) return {};
        return std::basic_string_view<CharT>(&_data[0], _data.size());
    }

    // iterators
    constexpr iterator       begin()       noexcept { return _data.begin(); }
    constexpr iterator       end()         noexcept { return _data.end(); }
    constexpr const_iterator begin() const noexcept { return _data.begin(); }
    constexpr const_iterator end()   const noexcept { return _data.end(); }

    // modifiers
    constexpr void push_back(CharT c) { _data.push_back(c); }
    constexpr void pop_back()         { _data.pop_back(); }

    constexpr basic_string &append(const CharT *s, size_type n) {
        append_raw(s, n);
        return *this;
    }
    constexpr basic_string &append(const CharT *s) {
        return append(s, len_of(s));
    }
    constexpr basic_string &append(std::basic_string_view<CharT> sv) {
        return append(sv.data(), sv.size());
    }
    constexpr basic_string &append(const basic_string &o) {
        for (size_type i = 0; i < o.size(); ++i) _data.push_back(o[i]);
        return *this;
    }

    constexpr basic_string &operator+=(CharT c) { _data.push_back(c); return *this; }
    constexpr basic_string &operator+=(const CharT *s) { return append(s); }
    constexpr basic_string &operator+=(std::basic_string_view<CharT> sv) { return append(sv); }
    constexpr basic_string &operator+=(const basic_string &o) { return append(o); }

    // search
    constexpr size_type find(CharT c, size_type from = 0) const {
        for (size_type i = from; i < _data.size(); ++i)
            if (_data[i] == c) return i;
        return npos;
    }
    constexpr size_type find(std::basic_string_view<CharT> needle,
                              size_type from = 0) const {
        size_type n = _data.size(), m = needle.size();
        if (m == 0) return from <= n ? from : npos;
        if (m > n) return npos;
        for (size_type i = from; i + m <= n; ++i) {
            size_type j = 0;
            while (j < m && _data[i + j] == needle[j]) ++j;
            if (j == m) return i;
        }
        return npos;
    }

    // Substring as a new basic_string (default-memory backend).
    constexpr basic_string substr(size_type pos, size_type count = npos) const {
        basic_string r;
        size_type avail = _data.size() > pos ? _data.size() - pos : 0;
        size_type take  = count < avail ? count : avail;
        r.reserve(take);
        for (size_type i = 0; i < take; ++i) r.push_back(_data[pos + i]);
        return r;
    }

    constexpr bool operator==(const basic_string &o) const {
        if (_data.size() != o._data.size()) return false;
        for (size_type i = 0; i < _data.size(); ++i)
            if (_data[i] != o._data[i]) return false;
        return true;
    }
    constexpr bool operator==(std::basic_string_view<CharT> sv) const {
        return view() == sv;
    }

    constexpr storage_type       &storage()       noexcept { return _data; }
    constexpr const storage_type &storage() const noexcept { return _data; }
};

} // namespace ycetl
