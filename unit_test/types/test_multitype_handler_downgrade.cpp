#include <boost/ut.hpp>
#include <tuple>
#include <ycetl/multitype_handler.hpp>
#include <ycetl/type_system.hpp>

using namespace boost::ut;
using namespace ycetl;

// Minimal handler implementation for testing purposes
template <typename T> struct dummy_handler {
  constexpr int value() const { return sizeof(T); }
};

suite multitype_handler_downgrade_suite = [] {
  "multitype_handler_downgrade_constructor"_test = [] {
    constexpr auto test = [] {
      using large_set = type_set<int, double, char>;
      using smaller_set = type_set<int, char>;

      multitype_handler<dummy_handler, large_set> large_handler;
      multitype_handler<dummy_handler, smaller_set> smaller_handler(
          large_handler);

      // Check explicitly that handlers were correctly picked
      return smaller_handler.get_handler<int>().value() == sizeof(int) &&
             smaller_handler.get_handler<char>().value() == sizeof(char);
    };

    static_assert(test());
    expect(test());
  };
};

int main() {}
