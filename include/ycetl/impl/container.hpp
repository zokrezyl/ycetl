#pragma once
#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/type_system.hpp>

// defines common traits for containers
namespace ycetl {

namespace container {

template <typename T> struct container_traits {
  // The fundamental unit of storage for the given ElementType.
  // This assumes backend_type_of_t is defined elsewhere and is generic enough.
  using storage_unit = backend_type_of_t<T>;
  using backend_type = dynamic_array<storage_unit>;

  // Type alias for relevant types for default_memory calculation.
  // This is typically the ElementType itself and its storage_unit.
  using relevant_of = relevant_types_t<T, storage_unit>;

  using default_memory = ::ycetl::default_memory<relevant_of>;

  // Add any other common types or deduction logic here if needed by various
  // containers For example, if you had a default backend type that was always
  // dynamic_array: using default_backend_type = dynamic_array<storage_unit>;
};

template <template <typename...> typename ContainerTemplate,
          typename T, // This is the element type of *this* container
          typename Memory = container_traits<T>::default_memory>
class container : public ycetl::template_info<ContainerTemplate, T, Memory> {

public:
  // The core element type this container logically holds.
  // If T is a nested container that can be memory-rebound,
  // its memory type is swapped for this container's Memory.
  // Otherwise, T is used as is.
  using value_type = std::conditional_t<
      ycetl::has_rebindable_memory_v<T>, // Condition: Is T a memory-rebindable
                                         // type?
      ycetl::rebind_memory_t<T, Memory>, // True: Rebind T's memory to current
                                         // container's Memory.
      T                                  // False: Use T as is.
      >;

  using memory_type = Memory;
  using storage_unit = typename container_traits<T>::storage_unit;
  using backend_type = typename container_traits<T>::backend_type;
  using relevant_of = typename container_traits<T>::relevant_of;

  // --- Static Asserts to verify consistency ---
  // These checks ensure that the rebinding mechanism does not alter the
  // fundamental storage characteristics of the nested type, only its allocator.

  // Check if the storage_unit derived from original T is the same as from the
  // (potentially) rebound value_type.
#if 0
  static_assert(
      std::is_same_v<typename container_traits<T>::storage_unit,
                     typename container_traits<value_type>::storage_unit>,
      "Container storage_unit mismatch after value_type rebinding. "
      "backend_type_of_t might be sensitive to allocator changes.");

  // Check if the backend_type derived from original T is the same as from the
  // (potentially) rebound value_type.
  static_assert(
      std::is_same_v<typename container_traits<T>::backend_type,
                     typename container_traits<value_type>::backend_type>,
      "Container backend_type mismatch after value_type rebinding. "
      "dynamic_array instantiation might be sensitive to allocator changes.");

  // Check if the relevant_of types derived from original T are the same as from
  // the (potentially) rebound value_type.
  static_assert(
      std::is_same_v<typename container_traits<T>::relevant_of,
                     typename container_traits<value_type>::relevant_of>,
      "Container relevant_of types mismatch after value_type rebinding. "
      "relevant_types_t might be sensitive to allocator changes.");
#endif
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
