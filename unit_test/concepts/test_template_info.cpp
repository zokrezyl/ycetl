// SPDX-License-Identifier: MIT

#include <cxxabi.h>
#include <iostream>
#include <memory>
#include <type_traits>
#include <typeinfo>

template <typename... Ts> struct type_set {
  template <template <typename...> typename T> using apply = T<Ts...>;
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

// Concatenates multiple type_sets into one.
template <typename...> struct type_set_concat;

// Base case: single type_set remains.
template <typename... Ts> struct type_set_concat<type_set<Ts...>> {
  using type = type_set<Ts...>;
};

// Concatenate two type_sets.
template <typename... Ts, typename... Us>
struct type_set_concat<type_set<Ts...>, type_set<Us...>> {
  using type = type_set<Ts..., Us...>;
};

// Recursive concatenation of multiple type_sets.
template <typename... Ts, typename... Us, typename... Rest>
struct type_set_concat<type_set<Ts...>, type_set<Us...>, Rest...> {
  using type = typename type_set_concat<type_set<Ts..., Us...>, Rest...>::type;
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

template <typename T> struct as_type_set {
  using type = type_set<T>;
};

// Specialization: Already a type_set.
template <typename... Ts> struct as_type_set<type_set<Ts...>> {
  using type = type_set<Ts...>;
};

// Helper alias to easily concatenate mixed types and type_sets.
template <typename... Args>
using type_set_concat_t =
    typename type_set_concat<typename as_type_set<Args>::type...>::type;

template <typename... Args>
using type_set_concat_t =
    typename type_set_concat<typename as_type_set<Args>::type...>::type;

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
struct rebind_template {
  using type = T_Rebindable; // Default: no rebind, return original type
};

template <typename T_Rebindable, typename NewTypeSet>
struct rebind_template<
    T_Rebindable, NewTypeSet,
    std::enable_if_t<is_template_rebindable_v<T_Rebindable>>> {
  using type =
      typename NewTypeSet::template apply<T_Rebindable::template template_type>;
};

template <typename T, typename = void>
struct has_memory_type : std::false_type {};

template <typename T>
struct has_memory_type<
    T, std::void_t<std::enable_if_t<std::is_class_v<T> || std::is_union_v<T>>,
                   typename T::memory_type>> : std::true_type {};

template <typename T>
inline constexpr bool has_memory_type_v = has_memory_type<T>::value;

struct WithMemory {
  using memory_type = int;
};
struct WithoutMemory {};

static_assert(has_memory_type_v<WithMemory>, "Should detect memory_type");
static_assert(!has_memory_type_v<WithoutMemory>,
              "Should not detect memory_type");
static_assert(!has_memory_type_v<int>,
              "Primitive types should not have memory_type");

// Class not inheriting from template_info
class B {};

template <typename... Args> class A : public template_info<A, Args...> {};

// Usage examples
static_assert(is_template_rebindable_v<A<int, double>>,
              "A should be template rebindable");
static_assert(!is_template_rebindable_v<B>,
              "B should NOT be template rebindable");
static_assert(!is_template_rebindable_v<int>,
              "int should NOT be template rebindable");

template <typename T>
inline constexpr bool has_rebindable_memory_v = is_template_rebindable_v<T>
                                             && has_memory_type_v<T>;

template <typename... Args>
struct Rebindable : template_info<Rebindable, Args...> {};
template <typename... Args>
struct RebindableWithMemory : template_info<RebindableWithMemory, Args...> {
  using memory_type = int;
};

static_assert(!has_rebindable_memory_v<int>,
              "int is neither rebindable nor has memory");
static_assert(!has_rebindable_memory_v<WithMemory>,
              "Has memory but not rebindable");
static_assert(!has_rebindable_memory_v<Rebindable<int>>,
              "Rebindable but no memory");
static_assert(has_rebindable_memory_v<RebindableWithMemory<int>>,
              "Both conditions satisfied");

// Example class
template <typename... Args> struct Example : template_info<Example, Args...> {};

template <typename> struct container_traits {
  using default_memory = std::allocator<void>; // trivial default
};

template <typename T_Rebindable, typename NewTypeSet>
using rebind_template_t =
    typename rebind_template<T_Rebindable, NewTypeSet>::type;

template <typename T_Rebindable, typename NewMemory,
          bool = is_template_rebindable_v<T_Rebindable>>
struct rebind_memory_helper {
  using type = T_Rebindable; // fallback if not rebindable
};

template <typename T_Rebindable, typename NewMemory>
struct rebind_memory_helper<T_Rebindable, NewMemory, true> {
  using type = rebind_template_t<
      T_Rebindable,
      type_set_concat_t<
          type_set_init_t<typename T_Rebindable::template_arguments>,
          type_set<NewMemory>>>;
};

template <typename T_Rebindable, typename NewMemory>
using rebind_memory_t =
    typename rebind_memory_helper<T_Rebindable, NewMemory>::type;

template <template <typename...> typename ContainerTemplate, typename T,
          typename Memory = typename container_traits<T>::default_memory>
class container : public template_info<ContainerTemplate, T, Memory> {

public:
  using value_type = std::conditional_t<has_rebindable_memory_v<T>,
                                        rebind_memory_t<T, Memory>, T>;
};

template <typename T,
          typename Memory = typename container_traits<T>::default_memory>
// clang-format on
class vector : public container<vector, T, Memory> {};

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

  vector<int> vec;

  return 0;
}
