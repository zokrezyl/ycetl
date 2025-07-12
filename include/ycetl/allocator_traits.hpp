#pragma once

#include <memory>
#include <type_traits>
#include <utility>

namespace ycetl {

template <typename Alloc>
struct allocator_traits : public std::allocator_traits<Alloc> {};

} // namespace ycetl
