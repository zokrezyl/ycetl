// SPDX-License-Identifier: MIT

#pragma once
#include <ycetl/cstddef.hpp>

#define AS_SIZE_T(x) static_cast<ycetl::size_t>(x)

// helper macros for values in N, meaning number of bytes etc
#define KN(x) (AS_SIZE_T(x) * 1024)
#define MN(x) (AS_SIZE_T(x) * 1024 * 1024)
#define GN(x) (AS_SIZE_T(x) * 1024 * 1024 * 1024)

namespace ycetl {

constexpr ycetl::size_t operator""_KB(unsigned long long x) { return x * 1024; }
constexpr ycetl::size_t operator""_MB(unsigned long long x) {
  return x * 1024 * 1024;
}

constexpr ycetl::size_t operator""_GB(unsigned long long x) {
  return x * 1024 * 1024 * 1024;
}

} // namespace ycetl
