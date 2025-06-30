#include "out.hpp"
#include <iostream>

int main() {

  static_assert(enums.size() == 57,
                "Number of enums does not match expected count");

  for (const auto &e : enums) {
    std::cout << e.first << ": " << std::endl;
    e.second(); // Call the handler
  }

  for (const auto &r : records) {
    std::cout << r.first << ": " << std::endl;
    r.second(); // Call the handler
  }

  for (const auto &t : typedefs) {
    std::cout << t.first << ": " << std::endl;
    t.second(); // Call the handler
  }
}
