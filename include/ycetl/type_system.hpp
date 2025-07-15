#pragma once
#include <type_traits>
#include <ycetl/types.hpp>

namespace ycetl {

template <typename... Ts> struct type_set {
  template <template <typename...> typename T> using apply = T<Ts...>;
};

// Concatenates multiple type_sets into one.
template <typename...> struct type_set_cat;

// Base case: single type_set remains.
template <typename... Ts> struct type_set_cat<type_set<Ts...>> {
  using type = type_set<Ts...>;
};

// Concatenate two type_sets.
template <typename... Ts, typename... Us>
struct type_set_cat<type_set<Ts...>, type_set<Us...>> {
  using type = type_set<Ts..., Us...>;
};

// Recursive concatenation of multiple type_sets.
template <typename... Ts, typename... Us, typename... Rest>
struct type_set_cat<type_set<Ts...>, type_set<Us...>, Rest...> {
  using type = typename type_set_cat<type_set<Ts..., Us...>, Rest...>::type;
};

// Converts a single type into a type_set if it's not already one.
template <typename T> struct as_type_set {
  using type = type_set<T>;
};

// Specialization: Already a type_set.
template <typename... Ts> struct as_type_set<type_set<Ts...>> {
  using type = type_set<Ts...>;
};

// Helper alias to easily concatenate mixed types and type_sets.
template <typename... Args>
using type_set_cat_t =
    typename type_set_cat<typename as_type_set<Args>::type...>::type;

// Checks if a type T is contained within a type_set.
template <typename T, typename Set> struct contains;

// Base case: Empty set does not contain any type.
template <typename T> struct contains<T, type_set<>> : std::false_type {};

// Recursively checks if T is the same as U or contained in Rest.
template <typename T, typename U, typename... Rest>
struct contains<T, type_set<U, Rest...>>
    : std::conditional_t<std::is_same_v<T, U>, std::true_type,
                         contains<T, type_set<Rest...>>> {};

// Removes duplicate types from a type_set.
template <typename InputSet, typename Result = type_set<>>
struct remove_duplicates;

// Base case: No more input types left, return the Result.
template <typename Result> struct remove_duplicates<type_set<>, Result> {
  using type = Result;
};

// Recursive case: Process each type, adding it to Result if unique.
template <typename Head, typename... Tail, typename... Rs>
struct remove_duplicates<type_set<Head, Tail...>, type_set<Rs...>> {
  using type = typename remove_duplicates<
      type_set<Tail...>,
      std::conditional_t<contains<Head, type_set<Rs...>>::value,
                         type_set<Rs...>, type_set<Rs..., Head>>>::type;
};

// Helper alias to simplify usage of remove_duplicates.
template <typename Set>
using remove_duplicates_t = typename remove_duplicates<Set>::type;

// Extracts relevant associated types from a type (if available).
template <typename T, typename = void> struct relevant_types_of {
  using type = type_set<T>; // Default: the type itself is relevant.
};

// Specialization if T provides a `relevant_of` type member.
template <typename T>
struct relevant_types_of<T, std::void_t<typename T::relevant_of>> {
  using type = typename T::relevant_of;
};

// Recursively flattens relevant types, preventing cycles.
template <typename T, typename Seen = type_set<>, typename = void>
struct flatten_relevant_types {
  using type = type_set<T>; // Base case: no further relevant types.
};

// Specialization: Flatten types having `relevant_of` member.
template <typename T, typename Seen>
struct flatten_relevant_types<T, Seen, std::void_t<typename T::relevant_of>> {
  using immediate = typename T::relevant_of;
  using type = typename flatten_relevant_types<immediate, Seen>::type;
};

// Specialization: Flatten a type_set recursively.
template <typename... Ts, typename Seen>
struct flatten_relevant_types<type_set<Ts...>, Seen> {
  using type = remove_duplicates_t<
      type_set_cat_t<typename flatten_relevant_types<Ts, Seen>::type...>>;
};

// Cycle detection specialization: stop recursion if type already seen.
template <typename T, typename Seen>
struct flatten_relevant_types<T, Seen,
                              std::enable_if_t<contains<T, Seen>::value>> {
  using type = type_set<>; // Stop further recursion to prevent cycles.
};

// User-friendly alias to retrieve all relevant, flattened, unique types.
template <typename... Args>
using relevant_types_t = remove_duplicates_t<
    typename flatten_relevant_types<type_set<Args...>>::type>;

// Convenience alias to extract relevant types from a single type.
template <typename T>
using relevant_types_of_t = typename relevant_types_of<T>::type;

} // namespace ycetl
