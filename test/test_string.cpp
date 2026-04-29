#include <ycetl/string.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/type_system.hpp>
#include <string>

using namespace ycetl;

constexpr auto get_string() {
    return basic_string<char>{"Hello, World!"};
}

constexpr auto add_to_string() {

  auto str = get_string();
  str += " How are you?";
  return str;

}

constexpr auto test_string() {
    auto str = add_to_string();
    return str.size();
}

constexpr int static_string() {
  string s1 = "Hello, World!";
  using relevant_types = relevant_types_t<string<>>;
  using string_static_memory_t = static_memory_t<relevant_types>;
  using static_string_t = string<string_static_memory_t>;
  string_static_memory_t memory;
  static_string_t s2("Hello, World!", memory);
  return 0;
}

constexpr int with_static_memory() {
  using relevant_types = relevant_types_t<string<>>;
  using string_static_memory_t = static_memory_t<relevant_types>;
  using static_string_t = string<string_static_memory_t>;
  string_static_memory_t memory;
  return 0;
}

constexpr auto get_static_string() {
  using relevant_types = relevant_types_t<string<>>;
  using string_static_memory_t = static_memory_t<relevant_types>;
  using static_string_t = string<string_static_memory_t>;
  string_static_memory_t memory;
  static_string_t s("Hello, World!", memory) ;
  return s;

}


int main() {

  constexpr auto result = test_string();
  constexpr auto result2 = get_static_string();


}
