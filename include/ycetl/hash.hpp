#pragma once

namespace ycetl {

namespace hash {
constexpr uint64_t fnv1a_hash(const std::string_view &str) {
  constexpr uint64_t offset_basis = 14695981039346656037ull;
  constexpr uint64_t prime = 1099511628211ull;

  uint64_t hash = offset_basis;
  for (size_t i = 0; i < str.size(); ++i) {
    hash ^= static_cast<uint8_t>(str[i]); // Ensures unsigned byte
    hash *= prime;
  }

  return hash;
}

} // namespace hash

}; // namespace ycetl
