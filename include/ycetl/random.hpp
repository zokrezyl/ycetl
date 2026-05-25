// SPDX-License-Identifier: MIT

#pragma once
#include <cstdint>

namespace ycetl {
namespace random {

struct fnv1a_hash {
  static constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;
  static constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;

  constexpr std::uint64_t operator()(std::uint64_t x) const {
    std::uint64_t hash = FNV_OFFSET_BASIS;

    // Process each byte of the input
    for (int i = 0; i < sizeof(std::uint64_t); ++i) {
      std::uint8_t byte = (x >> (i * 8)) & 0xFF;
      hash ^= byte;
      hash *= FNV_PRIME;
    }

    return hash;
  }
};

struct constexpr_random {
  std::uint64_t state;
  constexpr constexpr_random() {
    // __TIME__ format is "HH:MM:SS"
    const char *t = __TIME__;
    std::uint64_t h = (t[0] - '0') * 10 + (t[1] - '0');
    std::uint64_t m = (t[3] - '0') * 10 + (t[4] - '0');
    std::uint64_t s = (t[6] - '0') * 10 + (t[7] - '0');
    state = h * 3600 + m * 60 + s;
  }
};

template <typename DistributionFunc = fnv1a_hash,
          typename Seed = constexpr_random>
class random {
private:
  std::uint64_t state;
  DistributionFunc distribute;

public:
  // Constructor with default state
  constexpr random() : state(1) {}

  // Constructor with explicit seed
  constexpr explicit random(std::uint64_t seed) : state(seed == 0 ? 1 : seed) {}

  // Generate next value using the distribution function
  constexpr std::uint64_t next() {
    state = distribute(state);
    return state;
  }
};

constexpr random random_seed{};

} // namespace random
} // namespace ycetl
