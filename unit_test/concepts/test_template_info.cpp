#include <type_traits>

// general detection trait using void_t
template <typename, typename = void>
struct has_template_arguments : std::false_type {};

template <typename T>
struct has_template_arguments<T, std::void_t<typename T::template_arguments>>
    : std::true_type {};

template <typename, typename = void>
struct has_template_type : std::false_type {};

template <typename T>
struct has_template_type<T, std::void_t<typename T::template_type<int>>>
    : std::true_type {};

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

// Tests
static_assert(has_template_arguments<A<int, double>>::value,
              "A should have template_arguments");
static_assert(has_template_type<A<int, double>>::value,
              "A should have template_type");

static_assert(!has_template_arguments<B>::value,
              "B should NOT have template_arguments");
static_assert(!has_template_type<B>::value, "B should NOT have template_type");

static_assert(!has_template_type<int>::value,
              "int should NOT have template_type");
static_assert(!has_template_type<double>::value,
              "double should NOT have template_type");

int main() {
  // The static assertions will ensure that the code is correct at compile time.
  return 0;
}
