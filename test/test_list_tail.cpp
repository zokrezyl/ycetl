#include <iostream>
#include <type_traits>
#include <ycetl/impl/type_printer.hpp>

using namespace ycetl::print;

// A compile-time sequence of types
template <typename... Ts> struct type_set {};

// type_set_tail: all but the first element
template <typename Set> struct type_set_tail;

// Empty and singleton cases yield an empty type_set
template <> struct type_set_tail<type_set<>> {
  using type = type_set<>;
};

template <typename T> struct type_set_tail<type_set<T>> {
  using type = type_set<>;
};

// General case: strip off the first type
template <typename T, typename... Rest>
struct type_set_tail<type_set<T, Rest...>> {
  using type = type_set<Rest...>;
};

// Alias for easier usage
template <typename Set>
using type_set_tail_t = typename type_set_tail<Set>::type;

// Helper: concatenation of two type_sets
template <typename S1, typename S2> struct type_set_concat;

template <typename... A, typename... B>
struct type_set_concat<type_set<A...>, type_set<B...>> {
  using type = type_set<A..., B...>;
};

// type_set_init: all but the last element
template <typename Set> struct type_set_init;

// Empty and singleton cases yield an empty type_set
template <> struct type_set_init<type_set<>> {
  using type = type_set<>;
};

template <typename T> struct type_set_init<type_set<T>> {
  using type = type_set<>;
};

// Recursive case: take the first element and append init of the rest
template <typename T, typename... Rest>
struct type_set_init<type_set<T, Rest...>> {
  using type = typename type_set_concat<
      type_set<T>, typename type_set_init<type_set<Rest...>>::type>::type;
};

// Alias for easier usage
template <typename Set>
using type_set_init_t = typename type_set_init<Set>::type;

int main() {
  // Tests for type_set_tail_t
  static_assert(std::is_same<type_set_tail_t<type_set<>>, type_set<>>::value,
                "tail of empty should be empty");
  static_assert(std::is_same<type_set_tail_t<type_set<int>>, type_set<>>::value,
                "tail of singleton should be empty");
  static_assert(std::is_same<type_set_tail_t<type_set<int, char, double>>,
                             type_set<char, double>>::value,
                "tail of three should be <char, double>");

  // Tests for type_set_init_t
  static_assert(std::is_same<type_set_init_t<type_set<>>, type_set<>>::value,
                "init of empty should be empty");
  static_assert(std::is_same<type_set_init_t<type_set<int>>, type_set<>>::value,
                "init of singleton should be empty");
  static_assert(std::is_same<type_set_init_t<type_set<int, char, double>>,
                             type_set<int, char>>::value,
                "init of three should be <int, char>");

  std::cout << "All static assertions passed.\n";

  using input = type_set<int, double, char>;
  using tail = type_set_tail_t<input>;
  using tail = type_set_tail_t<input>;
  using init = type_set_init_t<input>;

  print<input>();
  print<tail>();
  print<init>();

  return 0;
}
