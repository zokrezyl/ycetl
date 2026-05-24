#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace ycetl {

// Compile-time-sized bitset, std::bitset-shaped. Backing storage is a
// std::array of 64-bit words rounded up — small, no allocation, fully
// constexpr, baked into the binary if used as a constexpr value.
//
// Bit ordering: bit i lives in word i/64 at position i%64. That matches
// std::bitset's documented behaviour (lowest-index bit in the lowest
// word's LSB), so to_string() / from a serialized form on disk would
// line up if anyone ever needs that.
template <std::size_t N> class bitset {
public:
  using size_type = std::size_t;
  using word_type = std::uint64_t;

private:
  static constexpr size_type bits_per_word = 64;
  static constexpr size_type word_count = (N + bits_per_word - 1)
                                        / bits_per_word;
  // At least one word so empty bitset<0> still has well-defined storage.
  static constexpr size_type storage_count = word_count == 0 ? 1 : word_count;

  std::array<word_type, storage_count> _words{};

  // Mask of bits actually inside the bitset within the last word —
  // used to discard the don't-care top bits after operations like
  // flip() or count() that touch every word.
  static constexpr word_type tail_mask() {
    if (N == 0 || N % bits_per_word == 0)
      return ~word_type{0};
    return (word_type{1} << (N % bits_per_word)) - 1;
  }

  constexpr void clear_tail() {
    if constexpr (word_count > 0)
      _words[word_count - 1] &= tail_mask();
  }

public:
  constexpr bitset() = default;

  // From a single 64-bit value — bits 0..63 take the low N bits.
  constexpr bitset(word_type v) {
    if constexpr (word_count > 0)
      _words[0] = v;
    clear_tail();
  }

  constexpr size_type size() const noexcept { return N; }

  constexpr bool test(size_type i) const {
    return (_words[i / bits_per_word] >> (i % bits_per_word)) & 1ULL;
  }
  constexpr bool operator[](size_type i) const { return test(i); }

  constexpr bitset &set(size_type i, bool v = true) {
    word_type mask = word_type{1} << (i % bits_per_word);
    if (v)
      _words[i / bits_per_word] |= mask;
    else
      _words[i / bits_per_word] &= ~mask;
    return *this;
  }
  constexpr bitset &set() {
    for (auto &w : _words)
      w = ~word_type{0};
    clear_tail();
    return *this;
  }

  constexpr bitset &reset(size_type i) { return set(i, false); }
  constexpr bitset &reset() {
    for (auto &w : _words)
      w = 0;
    return *this;
  }

  constexpr bitset &flip(size_type i) {
    _words[i / bits_per_word] ^= word_type{1} << (i % bits_per_word);
    return *this;
  }
  constexpr bitset &flip() {
    for (auto &w : _words)
      w = ~w;
    clear_tail();
    return *this;
  }

  constexpr size_type count() const noexcept {
    size_type c = 0;
    for (auto w : _words) {
      // Constexpr popcount — std::popcount is constexpr from
      // C++20 but only on libstdc++ recent enough; the loop here
      // works everywhere.
      while (w) {
        w &= w - 1;
        ++c;
      }
    }
    return c;
  }

  constexpr bool any() const noexcept {
    for (auto w : _words)
      if (w)
        return true;
    return false;
  }
  constexpr bool none() const noexcept { return !any(); }
  constexpr bool all() const noexcept { return count() == N; }

  constexpr bitset &operator&=(const bitset &o) {
    for (size_type i = 0; i < storage_count; ++i)
      _words[i] &= o._words[i];
    return *this;
  }
  constexpr bitset &operator|=(const bitset &o) {
    for (size_type i = 0; i < storage_count; ++i)
      _words[i] |= o._words[i];
    return *this;
  }
  constexpr bitset &operator^=(const bitset &o) {
    for (size_type i = 0; i < storage_count; ++i)
      _words[i] ^= o._words[i];
    clear_tail();
    return *this;
  }

  friend constexpr bitset operator&(bitset a, const bitset &b) {
    return a &= b;
  }
  friend constexpr bitset operator|(bitset a, const bitset &b) {
    return a |= b;
  }
  friend constexpr bitset operator^(bitset a, const bitset &b) {
    return a ^= b;
  }

  constexpr bitset operator~() const {
    bitset r = *this;
    r.flip();
    return r;
  }

  constexpr bool operator==(const bitset &o) const {
    for (size_type i = 0; i < storage_count; ++i)
      if (_words[i] != o._words[i])
        return false;
    return true;
  }
};

} // namespace ycetl
