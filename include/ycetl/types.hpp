#pragma once
#include <cstddef>

namespace ycetl {

template <typename... Ts> struct type_set {};

using default_allocator_value_type = std::byte;

} // namespace ycetl
