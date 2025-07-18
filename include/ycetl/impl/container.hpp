#pragma once
#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/type_system.hpp>

// defines common traits for containers
namespace ycetl {

namespace container {

struct by_value {};     // used to indicate that the container is by value
struct by_reference {}; // used to indicate that the container is by reference
//
template <typename T> struct container_traits_helper {
  using storage_unit = backend_type_of_t<T>;
  using backend_type_raw = dynamic_array<storage_unit>;
  using relevant_of = relevant_types_t<T, storage_unit>;
  using default_memory = ::ycetl::default_memory<relevant_of>;
};

template <template <typename...> typename ContainerTemplate, typename TypeSet,
          typename Memory =
              typename container_traits_helper<TypeSet>::default_memory,
          typename BackendMode = by_value>
struct container_traits;

// Specialization clearly isolating Memory and BackendMode from Args...
template <template <typename...> typename ContainerTemplate,
          typename... LeadingArgs, typename Memory, typename BackendMode>
struct container_traits<ContainerTemplate, type_set<LeadingArgs...>, Memory,
                        BackendMode>
    : public ycetl::template_info<ContainerTemplate, LeadingArgs..., Memory,
                                  BackendMode> {

  using memory_type = Memory;

  // Helper deduces relevant types from the first type (commonly value_type)
  using helper = container_traits_helper<first_type_t<LeadingArgs...>>;

  using storage_unit = typename helper::storage_unit;
  using backend_type_raw = typename helper::backend_type_raw;
  using backend_type = std::conditional_t<std::is_same_v<BackendMode, by_value>,
                                          backend_type_raw, backend_type_raw &>;

  using relevant_of = typename helper::relevant_of;
  using default_memory = typename helper::default_memory;

  // The "view_type" uses by_reference mode explicitly.
  using view_type = ContainerTemplate<LeadingArgs..., Memory, by_reference>;

  // The "value_type" is conditionally rebound to Memory, if possible.
  using value_type = std::conditional_t<
      ycetl::has_rebindable_memory_v<first_type_t<LeadingArgs...>>,
      ycetl::rebind_memory_t<first_type_t<LeadingArgs...>, Memory>,
      first_type_t<LeadingArgs...>>;

  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
};

template <template <typename...> typename ContainerTemplate, typename... Args>
class container {
public:
  using traits =
      container_traits<ContainerTemplate,
                       type_set_init_t<type_set_init_t<type_set<Args...>>>,
                       type_set_back_t<type_set_init_t<type_set<Args...>>>,
                       type_set_back_t<type_set<Args...>>>;

  using backend_type = typename traits::backend_type;
  using storage_unit = typename traits::storage_unit;
  using relevant_of = typename traits::relevant_of;
  using memory_type = typename traits::memory_type;
  using value_type = typename traits::value_type;
  using view_type = typename traits::view_type;

  using reference = typename traits::reference;
  using const_reference = typename traits::const_reference;
  using pointer = typename traits::pointer;
  using const_pointer = typename traits::const_pointer;
  using size_type = typename traits::size_type;
  using difference_type = typename traits::difference_type;
};

} // namespace container
} // namespace ycetl
