#pragma once
#include <ycetl/cstddef.hpp>

namespace ycetl {

constexpr std::size_t strlen(const char *str) {
  std::size_t len = 0;
  while (str[len] != '\0')
    ++len;
  return len;
}

template <typename T = ycetl::uint8_t>
constexpr void *memcpy(void *dst, const void *src, std::size_t size) {
  auto *d = static_cast<T *>(dst);
  auto *s = static_cast<const T *>(src);
  for (std::size_t i = 0; i < size; ++i)
    d[i] = s[i];
  return dst;
}

} // namespace ycetl
