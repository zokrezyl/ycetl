#pragma once
#include <ycetl/trivial_array.hpp>

namespace ycetl {

template <typename T, size_t N> class array {
  ycetl::trivial_array<T, N> data_;

public:
};

} // namespace ycetl
