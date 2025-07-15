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
template <template <typename> class HandlerImpl, typename TypeSet>
class multitype_handler;

template <template <typename> class HandlerImpl, typename... Ts>
class multitype_handler<HandlerImpl, type_set<Ts...>> {
private:
  std::tuple<HandlerImpl<Ts>...> _handlers;

public:
  using handled_types = type_set<Ts...>;
  constexpr multitype_handler() = default;

  template <typename T> constexpr HandlerImpl<T> &get_handler() {
    static_assert(type_in_typeset<T, type_set<Ts...>>::value,
                  "requested handler type is not present in the type_set");
    return std::get<HandlerImpl<T>>(_handlers);
  }

  template <typename T> constexpr const HandlerImpl<T> &get_handler() const {
    static_assert(type_in_typeset<T, type_set<Ts...>>::value,
                  "requested handler type is not present in the type_set");
    return std::get<HandlerImpl<T>>(_handlers);
  }

  template <typename... NewTs>
  constexpr auto downgrade_impl(type_set<NewTs...>) const {
    return multitype_handler<HandlerImpl, type_set<NewTs...>>{
        get_handler<NewTs>()...};
  }

  template <typename NewTypeSet> constexpr auto downgrade() const {
    return downgrade_impl(NewTypeSet{});
  }
};

}; // namespace ycetl
