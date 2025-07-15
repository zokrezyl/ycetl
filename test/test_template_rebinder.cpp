#include <ycetl/impl/type_printer.hpp>
#include <ycetl/template_rebinder.hpp>

struct default_allocator {};

template <typename T, typename Allocator = default_allocator>
class vector : public ycetl::template_rebinder<vector, T, Allocator> {};

constexpr auto get_simple_vector() {
  using vector_t = vector<int>;
  vector_t v1;
  return v1;
}

constexpr auto get_rebound_vector() {
  constexpr auto v1 = get_simple_vector();
  using vector_t = decltype(v1);
    // 1. Extract current arguments from vector_t
  using current_args = vector_t::template_arguments;

  // 2. Get all arguments except the last (remove allocator)
  using args_without_allocator = ycetl::type_set_init_t<current_args>;

  // 3. Append new allocator type to the arguments
  using new_args = ycetl::type_set_concat_t<args_without_allocator, std::allocator<int>>;

  // 4. Rebind the vector with the modified arguments
  using rebound_vector_t = new_args::apply<vector_t::template_type>;

  // Instantiate the rebound vector
  rebound_vector_t v2;

// Check results
#if 0
ycetl::print::print<decltype(v1)>();
ycetl::print::print<decltype(v2)>();
#endif

  return v2;
}

void test1() {
  std::cout << "\nRebound vector test1:\n";
  using vector_t = vector<int>;
  vector_t v1;
  std::cout << "v1: ";
  ycetl::print::print<decltype(v1)>();
  vector_t::template template_type<int> v2; // vector<int>
  using vector_derived_t = vector_t::template_arguments::apply<vector_t::template_type>;
  vector_derived_t v3; // vector<int>
  ycetl::print::print<decltype(v2)>();
  ycetl::print::print<decltype(v3)>();

}

void test2() {
  std::cout << "\nRebound vector test2:\n";
  constexpr auto v1 = get_simple_vector();
  std::cout << "v1: ";
  ycetl::print::print<decltype(v1)>();

  constexpr auto v2 = get_rebound_vector();
  std::cout << "v2: ";
  ycetl::print::print<decltype(v2)>();

}

int main() {
  test1();
  test2();

}
