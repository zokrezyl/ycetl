// SPDX-License-Identifier: MIT

#pragma once
#include <type_traits>

namespace ycetl {

template <typename, typename = void>
constexpr bool is_ycetl_object_type = false;

template <typename T>
constexpr bool
    is_ycetl_object_type<T, std::void_t<typename T::ycetl_object_type>> = true;

class default_context {};

template <typename T, typename ContextType = default_context> class object {
  T t;
  ContextType ctx; // this is an ultra thin object
  //
public:
  using ycetl_object_type = object<T, ContextType>;
};

template <typename T>
using deduced_object_type =
    std::conditional_t<is_ycetl_object_type<T>, typename T::ycetl_object_type,
                       object<T>>;

template <typename T, typename ContextType = default_context>
constexpr auto object_factory() -> deduced_object_type<T> {
  return {};
}

template <typename T, typename ContextType = default_context>
constexpr auto dummy_object() -> deduced_object_type<T> {
  if constexpr (is_ycetl_object_type<T>)
    return typename T::ycetl_object_type{};
  else
    return object<T, ContextType>{};
}

template <typename T, typename ContextType = default_context>
constexpr auto object_factory() -> deduced_object_type<T> {
  return dummy_object<T, ContextType>();
}

} // namespace ycetl
