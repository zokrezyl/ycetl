#pragma once
#include <type_traits>

#include <ycetl/types.hpp> // type_set

namespace ycetl {

/**
 * @brief Concept for types that define a `relevant_of` member type.
 *
 * This concept is satisfied if the type `T` provides a nested type alias
 * named `relevant_of`, which is used to derive the relevant (backend) type.
 */
template <typename T>
concept has_relevant_of = requires { typename T::relevant_of; };

//===----------------------------------------------------------------------===//
// type_set and utilities
//===----------------------------------------------------------------------===//

// Check if T is in Set
template <typename T, typename Set> struct contains;

template <typename T> struct contains<T, type_set<>> : std::false_type {};

template <typename T, typename Head, typename... Tail>
struct contains<T, type_set<Head, Tail...>>
    : std::conditional_t<std::is_same_v<T, Head>, std::true_type,
                         contains<T, type_set<Tail...>>> {};

//===----------------------------------------------------------------------===//
// relevant_type<T> using marker trait `T::relevant_of` if present
//===----------------------------------------------------------------------===//

template <typename T, typename = void> struct relevant_type {
  using type = T;
};

template <typename T>
struct relevant_type<T, std::void_t<typename T::relevant_of>> {
  using type = typename T::relevant_of;
};

//===----------------------------------------------------------------------===//
// map relevant_type<T> over a type_set<Ts...>
//===----------------------------------------------------------------------===//

template <typename TypeSet> struct map_relevant;

template <typename... Ts> struct map_relevant<type_set<Ts...>> {
  using type = type_set<typename relevant_type<Ts>::type...>;
};

//===----------------------------------------------------------------------===//
// remove_duplicates
//===----------------------------------------------------------------------===//

template <typename InputSet, typename Result = type_set<>>
struct remove_duplicates_impl;

template <typename Result> struct remove_duplicates_impl<type_set<>, Result> {
  using type = Result;
};

template <typename Head, typename... Tail, typename... Rs>
struct remove_duplicates_impl<type_set<Head, Tail...>, type_set<Rs...>> {
  using next = std::conditional_t<contains<Head, type_set<Rs...>>::value,
                                  type_set<Rs...>, type_set<Rs..., Head>>;

  using type = typename remove_duplicates_impl<type_set<Tail...>, next>::type;
};

template <typename Set> struct remove_duplicates {
  using type = typename remove_duplicates_impl<Set>::type;
};

//===----------------------------------------------------------------------===//
// Final composition
//===----------------------------------------------------------------------===//

template <typename Input> struct relevant_types {
private:
  using mapped = typename map_relevant<Input>::type;

public:
  using type = typename remove_duplicates<mapped>::type;
};

template <typename T> using relevant_types_t = typename relevant_types<T>::type;

/**
 * @brief Forces compile-time resolution of relevant types from a type_set.
 *
 * This function triggers the instantiation of `relevant_types_t<T>` in a
 * `consteval` context, ensuring that all type transformations (e.g.,
 * deduplication and recursive resolution via `relevant_of`) are performed at
 * compile time.
 *
 * Use this to catch structural or semantic errors early, and to validate
 * correctness of the type system in constexpr-enabled environments.
 *
 * @tparam T A `type_set<...>` representing the input types.
 * @return A default-initialized value of the resulting relevant type set
 * (unused).
 */
template <typename T>
consteval auto test_relevant_types() -> relevant_types_t<T> {
  return {};
}

} // namespace ycetl
