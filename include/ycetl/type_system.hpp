#pragma once
#include <type_traits>

#include <ycetl/types.hpp> // type_set
namespace ycetl {
//===----------------------------------------------------------------------===//
// type_set and utilities
//===----------------------------------------------------------------------===//

// template <typename... Ts> struct type_set {};

// Check if T is in Set
template <typename T, typename Set> struct contains;

template <typename T> struct contains<T, type_set<>> : std::false_type {};

template <typename T, typename Head, typename... Tail>
struct contains<T, type_set<Head, Tail...>>
    : std::conditional_t<std::is_same_v<T, Head>, std::true_type,
                         contains<T, type_set<Tail...>>> {};

//===----------------------------------------------------------------------===//
// backend_type<T> using marker trait `T::backend_of` if present
//===----------------------------------------------------------------------===//

template <typename T, typename = void> struct backend_type {
  using type = T;
};

template <typename T>
struct backend_type<T, std::void_t<typename T::backend_of>> {
  using type = typename T::backend_of;
};

//===----------------------------------------------------------------------===//
// map backend_type<T> over a type_set<Ts...>
//===----------------------------------------------------------------------===//

template <typename TypeSet> struct map_backend;

template <typename... Ts> struct map_backend<type_set<Ts...>> {
  using type = type_set<typename backend_type<Ts>::type...>;
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

template <typename Input> struct working_type_set {
private:
  using mapped = typename map_backend<Input>::type;

public:
  using type = typename remove_duplicates<mapped>::type;
};

} // namespace ycetl
