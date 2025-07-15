#include <iostream>
#include <type_traits>
#include <ycetl/impl/type_printer.hpp>
#include <ycetl/type_system.hpp>


using namespace ycetl;

// ---------------------------------------------------------------------

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
  using init = type_set_init_t<input>;
  using back = type_set_back_t<input>;

  ycetl::print::print<input>(); // prints: int, double, char
  //
  #if 0 
  print<tail>();  // prints: double, char
  print<init>();  // prints: int, double
  print<back>();  // prints: char
  #endif

  return 0;
}
