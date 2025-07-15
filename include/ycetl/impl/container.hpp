#pragma once
#include <ycetl/dynamic_array.hpp>
#include <ycetl/impl/container_traits.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/type_system.hpp>

// defines common traits for containers
namespace ycetl {
namespace container {

// Common base container trait
template <typename T,
          typename Memory =
              default_memory<relevant_types_t<T, backend_type_of_t<T>>>>
class container {
public:
  using storage_unit = backend_type_of_t<T>;
  using backend_type = dynamic_array<storage_unit>;
  using relevant_of = relevant_types_t<storage_unit, T>;
  using default_memory =
      ::ycetl::default_memory<relevant_types_t<T, backend_type_of_t<T>>>;
};

#if 0
// example Derived containers
template <typename T, typename Memory>
struct vector : container_traits<T, Alloccator> {};

template <typename T, typename Memory>
struct set : container_traits<T, Memory> {};

#endif

} // namespace container
} // namespace ycetl
