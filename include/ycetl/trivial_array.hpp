#pragma once
#include <ycetl/cstddef.hpp>

namespace ycetl {

// simple std::array like container
template <typename T, ycetl::size_t N> struct trivial_array {
  T buffer[N];
  constexpr trivial_array() : buffer() {};

  constexpr T *memory() noexcept { return static_cast<T *>(buffer); }
  // constexpr _array(): bufer {};
  constexpr T *data() noexcept { return buffer; }
  constexpr const T *data() const noexcept { return buffer; }
  constexpr ycetl::size_t size() const noexcept { return N; }

  constexpr T &operator[](ycetl::size_t i) noexcept { return buffer[i]; }
  constexpr const T &operator[](ycetl::size_t i) const noexcept {
    return buffer[i];
  }

  /*
  constexpr T *begin() noexcept { return buffer; }
  constexpr T *end() noexcept { return buffer + N; }
  constexpr const T *begin() const noexcept { return buffer; }
  constexpr const T *end() const noexcept { return buffer + N; }
  */
};

} // namespace ycetl
