#pragma once
#include <ycetl/type_system.hpp>

namespace ycetl {

template <template <typename... Args> typename Template, typename... Ts>
class template_rebinder {
public:
  template <typename... Args> using template_type = Template<Args...>;
  using template_arguments = type_set<Ts...>;
};

} // namespace ycetl
