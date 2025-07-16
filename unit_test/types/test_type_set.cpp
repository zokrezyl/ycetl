#include <boost/ut.hpp>
#include <ycetl/type_system.hpp>

using namespace boost::ut;
using namespace ycetl;

// This test suite explicitly tests the issue causing the incomplete type error
// involving `type_set_init` when used with `template_arguments_t`.
template <typename... Ts>
struct Rebindable : template_info<Rebindable, Ts...> {};

suite type_set_init_with_template_arguments_suite = [] {
  // Define a test type with template_arguments_t

  "type_set_init_with_template_arguments_single"_test = [] {
    constexpr auto test = [] {
      using args = template_arguments_t<int>;
      using init = type_set_init_t<args>; // Should be empty
      return std::is_same_v<init, type_set<>>;
    };
    expect(test());
  };

  "type_set_init_with_template_arguments_multiple"_test = [] {
    constexpr auto test = [] {
      using args = template_arguments_t<int, double, char>;
      using init = type_set_init_t<args>; // Should be type_set<int, double>
      return std::is_same_v<init, type_set<int, double>>;
    };
    expect(test());
  };

  "type_set_init_with_template_arguments_empty"_test = [] {
    constexpr auto test = [] {
      using args = template_arguments_t<>;
      using init = type_set_init_t<args>; // Should be empty
      return std::is_same_v<init, type_set<>>;
    };
    expect(test());
  };
};

int main() {}
