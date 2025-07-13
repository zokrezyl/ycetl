#pragma once
#include <type_traits>
#include <ycetl/types.hpp>

namespace ycetl {

// Type Set Concatenation

template <typename...> struct type_set_cat;

template <typename... Ts> struct type_set_cat<type_set<Ts...>> {
  using type = type_set<Ts...>;
};

template <typename... Ts, typename... Us>
struct type_set_cat<type_set<Ts...>, type_set<Us...>> {
  using type = type_set<Ts..., Us...>;
};

template <typename... Ts, typename... Us, typename... Rest>
struct type_set_cat<type_set<Ts...>, type_set<Us...>, Rest...> {
  using type = typename type_set_cat<type_set<Ts..., Us...>, Rest...>::type;
};

// Helper to convert single type into type_set
template <typename T> struct as_type_set {
  using type = type_set<T>;
};

template <typename... Ts> struct as_type_set<type_set<Ts...>> {
  using type = type_set<Ts...>;
};

template <typename... Args>
using type_set_cat_t =
    typename type_set_cat<typename as_type_set<Args>::type...>::type;

// Remove duplicates

template <typename T, typename Set> struct contains;

template <typename T> struct contains<T, type_set<>> : std::false_type {};

template <typename T, typename U, typename... Rest>
struct contains<T, type_set<U, Rest...>>
    : std::conditional_t<std::is_same_v<T, U>, std::true_type,
                         contains<T, type_set<Rest...>>> {};

template <typename InputSet, typename Result = type_set<>>
struct remove_duplicates;

template <typename Result> struct remove_duplicates<type_set<>, Result> {
  using type = Result;
};

template <typename Head, typename... Tail, typename... Rs>
struct remove_duplicates<type_set<Head, Tail...>, type_set<Rs...>> {
  using type = typename remove_duplicates<
      type_set<Tail...>,
      std::conditional_t<contains<Head, type_set<Rs...>>::value,
                         type_set<Rs...>, type_set<Rs..., Head>>>::type;
};

template <typename Set>
using remove_duplicates_t = typename remove_duplicates<Set>::type;

// Relevant type extraction

template <typename T, typename = void> struct relevant_types_of {
  using type = type_set<T>;
};

template <typename T>
struct relevant_types_of<T, std::void_t<typename T::relevant_of>> {
  using type = typename T::relevant_of;
};

// Recursive mapping and flattening

template <typename T> struct flatten_relevant_types {
  using immediate = typename relevant_types_of<T>::type;
  using type = immediate;
};

template <typename... Ts> struct flatten_relevant_types<type_set<Ts...>> {
  using type = type_set_cat_t<typename flatten_relevant_types<Ts>::type...>;
};

// Main relevant_types implementation

template <typename... Args>
using relevant_types_t = remove_duplicates_t<
    typename flatten_relevant_types<type_set<Args...>>::type>;

// Helper alias
template <typename T>
using relevant_types_of_t = typename relevant_types_of<T>::type;

} // namespace ycetl
