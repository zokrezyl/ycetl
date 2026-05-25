// SPDX-License-Identifier: MIT

#include <boost/ut.hpp>
#include <type_traits> // For std::is_same_v, std::is_class_v, std::is_union_v, std::void_t, std::enable_if_t

// --- YCETL Type System Components (as provided by user) ---

// General detection trait using void_t
template <typename, typename = void>
struct is_template_rebindable : std::false_type {};

template <typename T>
struct is_template_rebindable<T, std::void_t<typename T::template_arguments>>
    : std::true_type {};

// Helper inline variable for convenience
template <typename T>
inline constexpr bool is_template_rebindable_v =
    is_template_rebindable<T>::value;

// Example classes
template <typename... Ts> struct template_arguments_t {
  template <template <typename...> typename T> using apply = T<Ts...>;
};

template <template <typename...> typename Template, typename... Ts>
struct template_info {
  template <typename... Args> using template_type = Template<Args...>;
  using template_arguments = template_arguments_t<Ts...>;
};

// Class that inherits from template_info
template <typename... Args> class A : public template_info<A, Args...> {};

// Class not inheriting from template_info
class B {};

template <typename T_Rebindable, typename NewTypeSet, typename Enable = void>
struct rebind_template {};

template <typename T_Rebindable, typename NewTypeSet>
struct rebind_template<
    T_Rebindable, NewTypeSet,
    std::enable_if_t<is_template_rebindable_v<T_Rebindable>>> {
  using type =
      typename NewTypeSet::template apply<T_Rebindable::template template_type>;
};

// Example for rebind_template test
template <typename... Args> struct Example : template_info<Example, Args...> {};

// --- Boost.UT Test Cases ---
using namespace boost::ut;

suite type_system_tests = [] {
  "is_template_rebindable_v_trait"_test = [] {
    // Test cases directly from user's static_asserts
    expect(is_template_rebindable_v<A<int, double>>);
    expect(!is_template_rebindable_v<B>);
    expect(!is_template_rebindable_v<int>);
    expect(!is_template_rebindable_v<double>);
  };

  "rebind_template_action"_test = [] {
    // Rebind Example<int, float> to Example<double, bool>
    using original = Example<int, float>;
    using new_types = template_arguments_t<double, bool>;

    using rebound = rebind_template<original, new_types>::type;

    // Test case directly from user's static_assert
    expect(std::is_same_v<rebound, Example<double, bool>>);
  };
};

int main(int, char **) { return 0; }
