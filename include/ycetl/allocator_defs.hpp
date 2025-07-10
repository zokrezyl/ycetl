#pragma once
#include <cstddef>

// we define here shared types, values etc. for allocators and memory management
namespace ycetl {
namespace memory {

using default_allocator_value_type = std::byte;

// TODO rename this into something more appropriate
template <typename ForwardIt, typename Alloc>
constexpr void destroy_range_with_alloc(ForwardIt first, ForwardIt last,
                                        Alloc &alloc) {
  for (; first != last; ++first)
    std::allocator_traits<Alloc>::destroy(alloc, std::addressof(*first));
}

template <typename InputIt, typename OutputIt, typename Alloc>
constexpr OutputIt uninitialized_move(InputIt first, InputIt last,
                                      OutputIt dest, Alloc &alloc) {
  for (; first != last; ++first, ++dest)
    std::allocator_traits<Alloc>::construct(alloc, std::addressof(*dest),
                                            std::move(*first));
  return dest;
}

template <typename InputIt, typename OutputIt, typename Alloc>
OutputIt uninitialized_copy(InputIt first, InputIt last, OutputIt dest,
                            Alloc &alloc) {
  for (; first != last; ++first, (void)++dest)
    std::allocator_traits<Alloc>::construct(alloc, std::addressof(*dest),
                                            *first);
  return dest;
}

} // namespace memory
}; // namespace ycetl
