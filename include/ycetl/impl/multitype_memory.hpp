#pragma once
#include <cstddef>

#include <ycetl/impl/multitype_handler.hpp>
#include <ycetl/type_system.hpp>

namespace ycetl {

template <template <typename> class MemoryBackend, typename... RawType>
class multitype_memory : public multitype_handler<MemoryBackend, RawType...> {
public:
  using base_type = multitype_handler<MemoryBackend, RawType...>;
  using base_type::base_type;
  using type_set = typename base_type::types;

  template <typename T> constexpr T *allocate(std::size_t n) {
    return this->template get_handler<T>().allocate(n);
  }

  template <typename T> constexpr void deallocate(T *p, std::size_t n) {
    this->template get_handler<T>().deallocate(p, n);
  }
};

} // namespace ycetl
