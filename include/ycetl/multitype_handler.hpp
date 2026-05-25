// SPDX-License-Identifier: MIT

#pragma once
#include <tuple>
#include <ycetl/type_system.hpp>

namespace ycetl {

template <typename T, typename TypeSet> struct type_in_typeset;

template <typename T, typename... Ts>
struct type_in_typeset<T, type_set<Ts...>>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

/*
The `multitype_handler` exists primarily to support the operations of
`multitype_storage`. It provides mechanisms to operate uniformly across
different types at compile-time, enabling type-safe operations within the
constexpr context where traditional type erasure techniques are restricted.

A crucial constraint in constexpr computation is that while casting to `void*`
is theoretically possible, casting back from `void*` to a typed pointer
(`static_cast<T*>`) is forbidden. This means standard type-erasure techniques
that work at runtime fail in constexpr contexts.
*/

// the subclass that explicitly takes expanded, flattened type_set
template <template <typename> class HandlerImpl, typename FlatTypeSet>
class multitype_handler_impl;

template <template <typename> class HandlerImpl, typename... Ts>
class multitype_handler_impl<HandlerImpl, type_set<Ts...>> {
public:
  using types = type_set<Ts...>;
  using handled_types = type_set<HandlerImpl<Ts>...>;

private:
  std::tuple<HandlerImpl<Ts>...> _handlers;

public:
  constexpr multitype_handler_impl() = default;

  template <typename LargerSet>
  constexpr multitype_handler_impl(
      const multitype_handler_impl<HandlerImpl, LargerSet> &larger_handler)
      : _handlers(larger_handler.template get_handler<Ts>()...) {}

  template <typename T> constexpr HandlerImpl<T> &get_handler() {
    static_assert(contains<T, types>::value,
                  "Requested handler type is not present in the type_set");
    return std::get<HandlerImpl<T>>(_handlers);
  }

  template <typename T> constexpr const HandlerImpl<T> &get_handler() const {
    static_assert(contains<T, types>::value,
                  "Requested handler type is not present in the type_set");
    return std::get<HandlerImpl<T>>(_handlers);
  }
};

// the primary class that calculates types, then explicitly applies and forwards
template <template <typename> class HandlerImpl, typename... RawTypes>
class multitype_handler
    : public multitype_handler_impl<HandlerImpl, flat_type_set_t<RawTypes...>> {

  using Base =
      multitype_handler_impl<HandlerImpl, flat_type_set_t<RawTypes...>>;

public:
  using types = typename Base::types;
  using handled_types = typename Base::handled_types;

  constexpr multitype_handler() = default;

  template <typename... LargerRawTypes>
  constexpr multitype_handler(
      const multitype_handler<HandlerImpl, LargerRawTypes...> &larger_handler)
      : Base(larger_handler) {}
};

}; // namespace ycetl
