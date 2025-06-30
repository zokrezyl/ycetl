#pragma once

namespace ycetl {

constexpr std::size_t strlen(const char *str) {
  std::size_t len = 0;
  while (str[len] != '\0')
    ++len;
  return len;
}

} // namespace ycetl
