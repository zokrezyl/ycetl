#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace ycetl {

// FNV-1a — kept at namespace scope (it used to live in a `hash`
// sub-namespace, but that collided with the new `template <T> struct
// hash` below — both can't share the name `ycetl::hash`).
constexpr std::uint64_t fnv1a_hash(std::string_view str) noexcept {
    constexpr std::uint64_t offset_basis = 14695981039346656037ull;
    constexpr std::uint64_t prime        = 1099511628211ull;

    std::uint64_t h = offset_basis;
    for (std::size_t i = 0; i < str.size(); ++i) {
        h ^= static_cast<std::uint8_t>(str[i]);
        h *= prime;
    }
    return h;
}

// Drop-in constexpr replacement for std::hash<T>. We need our own
// because libstdc++ marks std::hash<int>::operator() non-constexpr,
// which breaks any hash-based container we want to evaluate at compile
// time. Specialise for additional T as needed.
template <typename T> struct hash;

namespace hash_detail {
// Wang's 64-bit integer mix — constexpr-clean, decent distribution.
constexpr std::size_t mix64(std::uint64_t x) noexcept {
    x = (~x) + (x << 21);
    x = x ^ (x >> 24);
    x = (x + (x << 3)) + (x << 8);
    x = x ^ (x >> 14);
    x = (x + (x << 2)) + (x << 4);
    x = x ^ (x >> 28);
    x = x + (x << 31);
    return static_cast<std::size_t>(x);
}
} // namespace hash_detail

#define YCETL_DEFINE_INT_HASH(T)                                              \
    template <> struct hash<T> {                                              \
        constexpr std::size_t operator()(T v) const noexcept {                \
            return hash_detail::mix64(static_cast<std::uint64_t>(v));         \
        }                                                                     \
    }

YCETL_DEFINE_INT_HASH(bool);
YCETL_DEFINE_INT_HASH(char);
YCETL_DEFINE_INT_HASH(signed char);
YCETL_DEFINE_INT_HASH(unsigned char);
YCETL_DEFINE_INT_HASH(short);
YCETL_DEFINE_INT_HASH(unsigned short);
YCETL_DEFINE_INT_HASH(int);
YCETL_DEFINE_INT_HASH(unsigned int);
YCETL_DEFINE_INT_HASH(long);
YCETL_DEFINE_INT_HASH(unsigned long);
YCETL_DEFINE_INT_HASH(long long);
YCETL_DEFINE_INT_HASH(unsigned long long);

#undef YCETL_DEFINE_INT_HASH

template <typename T> struct hash<T *> {
    constexpr std::size_t operator()(T *p) const noexcept {
        return hash_detail::mix64(reinterpret_cast<std::uintptr_t>(p));
    }
};

template <> struct hash<std::string_view> {
    constexpr std::size_t operator()(std::string_view s) const noexcept {
        return static_cast<std::size_t>(fnv1a_hash(s));
    }
};

} // namespace ycetl
