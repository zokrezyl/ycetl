#include <cxxabi.h>
#include <iostream>
#include <memory>
#include <type_traits>
#include <typeinfo>

// Helper function for demangling type names at runtime
std::string demangle(const char *name) {
  int status = -1;
  std::unique_ptr<char, void (*)(void *)> res{
      abi::__cxa_demangle(name, NULL, NULL, &status), std::free};
  return (status == 0) ? res.get() : name;
}

// --- Your trait and rebinding logic ---
template <typename, typename = void>
struct is_template_rebindable : std::false_type {};

template <typename T>
struct is_template_rebindable<T, std::void_t<typename T::template_arguments>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_template_rebindable_v =
    is_template_rebindable<T>::value;

template <typename... Ts> struct template_arguments_t {
  template <template <typename...> typename T> using apply = T<Ts...>;
};

template <template <typename...> typename Template, typename... Ts>
struct template_info {
  template <typename... Args> using template_type = Template<Args...>;
  using template_arguments = template_arguments_t<Ts...>;
};

template <typename T_Rebindable, typename NewTypeSet, typename Enable = void>
struct rebind_template {};

template <typename T_Rebindable, typename NewTypeSet>
struct rebind_template<
    T_Rebindable, NewTypeSet,
    std::enable_if_t<is_template_rebindable_v<T_Rebindable>>> {
  using type =
      typename NewTypeSet::template apply<T_Rebindable::template template_type>;
};

template <typename... Args> class A : public template_info<A, Args...> {};

// Class not inheriting from template_info
class B {};

// Usage examples
static_assert(is_template_rebindable_v<A<int, double>>,
              "A should be template rebindable");
static_assert(!is_template_rebindable_v<B>,
              "B should NOT be template rebindable");
static_assert(!is_template_rebindable_v<int>,
              "int should NOT be template rebindable");

// Example class
template <typename... Args> struct Example : template_info<Example, Args...> {};

// --- Runtime test in main ---
int main() {
  using original = Example<int, float>;
  using new_types = template_arguments_t<double, bool>;

  using rebound = rebind_template<original, new_types>::type;

  std::cout << "Original type: " << demangle(typeid(original).name()) << '\n';
  std::cout << "Rebound type : " << demangle(typeid(rebound).name()) << '\n';

  if constexpr (std::is_same_v<rebound, Example<double, bool>>)
    std::cout << "Runtime check: Rebinding succeeded!\n";
  else
    std::cout << "Runtime check: Rebinding failed!\n";

  return 0;
}
