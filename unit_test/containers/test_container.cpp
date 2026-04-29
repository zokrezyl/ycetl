#include <boost/ut.hpp>
#include <ycetl/dynamic_array.hpp>
#include <ycetl/impl/container.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/type_system.hpp>

using namespace boost::ut;
using namespace ycetl;

namespace {

// Simple test container templates for demonstration purposes
template <typename T, typename Memory, typename BackendMode>
class my_vector
    : public container::container<my_vector, T, Memory, BackendMode> {};

template <typename CharT, typename Traits, typename Memory,
          typename BackendMode>
class my_basic_string
    : public container::container<my_basic_string, CharT, Traits, Memory,
                                  BackendMode> {};

template <typename Memory>
struct memory_rebindable_type
    : ycetl::template_info<memory_rebindable_type, Memory> {
  using memory_type = Memory;

  template <typename NewMemory>
  using rebind = memory_rebindable_type<NewMemory>;
};

suite container_traits_suite = [] {
  "vector_container_traits_0"_test = [] {
    constexpr auto test = [] {
      using vec_traits =
          container::container_traits<my_vector, type_set<int>,
                                      container::by_value, default_memory<int>>;

      return std::is_same_v<vec_traits::value_type, int>;
    };
    static_assert(test());
    expect(test());
  };
  "vector_container_traits_1"_test = [] {
    constexpr auto test = [] {
      using vec_traits =
          container::container_traits<my_vector, type_set<int>,
                                      container::by_value, default_memory<int>>;

      return std::is_same_v<vec_traits::backend_type_raw, dynamic_array<int>>;
    };
    static_assert(test());
    expect(test());
  };
  "vector_container_traits_2"_test = [] {
    constexpr auto test = [] {
      using vec_traits =
          container::container_traits<my_vector, type_set<int>,
                                      container::by_value, default_memory<int>>;

      return std::is_same_v<vec_traits::backend_type, dynamic_array<int>>;
    };
    static_assert(test());
    expect(test());
  };
  "vector_container_traits_3"_test = [] {
    constexpr auto test = [] {
      using vec_traits =
          container::container_traits<my_vector, type_set<int>,
                                      container::by_value, default_memory<int>>;

      return std::is_same_v<vec_traits::memory_type, default_memory<int>>;
    };
    static_assert(test());
    expect(test());
  };

  "string_container_traits"_test = [] {
    constexpr auto test = [] {
      using string_traits = container::container_traits<
          my_basic_string, type_set<char, std::char_traits<char>>,
          default_memory<char>, container::by_reference>;

      return std::is_same_v<string_traits::value_type, char> &&
             std::is_same_v<string_traits::backend_type_raw,
                            dynamic_array<char>> &&
             std::is_same_v<string_traits::backend_type,
                            dynamic_array<char> &> &&
             std::is_same_v<string_traits::memory_type, default_memory<char>>;
    };
    static_assert(test());
    expect(test());
  };

  "rebindable_value_type"_test = [] {
    constexpr auto test = [] {
      using traits =
          container::container_traits<my_vector,
                                      type_set<memory_rebindable_type<void>>,
                                      default_memory<int>, container::by_value>;

      return std::is_same_v<traits::value_type,
                            memory_rebindable_type<default_memory<int>>> &&
             ycetl::has_rebindable_memory_v<memory_rebindable_type<void>>;
    };
    static_assert(test());
    expect(test());
  };
};

} // namespace
//
int main() {}
