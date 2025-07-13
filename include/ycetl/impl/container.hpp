#pragma once
#include <ycetl/dynamic_array.hpp>
#include <ycetl/impl/container_traits.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/relevant_types.hpp>

// defines common traits for containers
namespace ycetl {
namespace container {

// Common base container trait
template <typename T,
          typename Allocator =
              default_allocator<relevant_types_t<T, storage_type_of_t<T>>>>
class container {
public:
  using storage_unit = storage_type_of_t<T>;
  using storage_type = dynamic_array<storage_unit>;
  using relevant_of = relevant_types_t<storage_unit, T>;
  using default_allocator =
      ::ycetl::default_allocator<relevant_types_t<T, storage_type_of_t<T>>>;
};

#if 0
// example Derived containers
template <typename T, typename Allocator>
struct vector : container_traits<T, Alloccator> {};

template <typename T, typename Allocator>
struct set : container_traits<T, Allocator> {};

#endif

} // namespace container
} // namespace ycetl
