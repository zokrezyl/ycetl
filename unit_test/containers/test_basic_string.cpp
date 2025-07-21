#include <boost/ut.hpp>
#include <ycetl/basic_string.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/string.hpp>
#include <ycetl/typed_dynamic_memory.hpp>

using namespace boost::ut;
using namespace ycetl;

suite basic_string_suite_no_explicit_memory = [] {
  "empty_basic_string_default_memory"_test = [] {
    constexpr auto test = [] {
      basic_string<char> s;
      return s.empty() && s.size() == 0_u;
    };
    static_assert(test());
    expect(test());
  };

  "construct_from_literal_default_memory"_test = [] {
    constexpr auto test = [] {
      basic_string<char> s("hello");
      return s.size() == 5_u && s[0] == 'h' && s[4] == 'o';
    };
    static_assert(test());
    expect(test());
  };

  "push_back_default_memory"_test = [] {
    constexpr auto test = [] {
      basic_string<char> s;
      s.push_back('x');
      s.push_back('y');
      return s.size() == 2_u && s[0] == 'x' && s[1] == 'y';
    };
    static_assert(test());
    expect(test());
  };

  "copy_construct_default_memory"_test = [] {
    constexpr auto test = [] {
      basic_string<char> a("abc");
      basic_string<char> b = a;
      return b.size() == 3_u && b[0] == 'a' && b[2] == 'c';
    };
    static_assert(test());
    expect(test());
  };

  "move_construct_default_memory"_test = [] {
    constexpr auto test = [] {
      basic_string<char> a("xy");
      basic_string<char> b = std::move(a);
      return b.size() == 2_u && b[0] == 'x' && b[1] == 'y';
    };
    static_assert(test());
    expect(test());
  };

  "clear_and_reuse_default_memory"_test = [] {
    constexpr auto test = [] {
      basic_string<char> s("test");
      s.clear();
      s.push_back('z');
      return s.size() == 1_u && s[0] == 'z';
    };
    static_assert(test());
    expect(test());
  };

  "initializer_list_default_memory"_test = [] {
    constexpr auto test = [] {
      basic_string<char> s({'a', 'b', 'c'});
      return s.size() == 3_u && s[1] == 'b';
    };
    static_assert(test());
    expect(test());
  };
  "return_to_runtime"_test = [] {
    constexpr auto test = [] {
      using string_t = static_t<basic_string<char>>;
      // string_t s({'a', 'b', 'c'});
      string_t s;
      s.reserve(3);
      // return s.size() == 3_u && s[1] == 'b';
      //
      static_assert(std::is_same_v<typename string_t::value_type, char>);
      static_assert(has_rebindable_memory_v<string_t>);

      static_assert(std::is_same_v<typename string_t::value_type, char>);
      static_assert(has_rebindable_memory_v<string_t>);
      static_assert(
          std::is_same_v<typename string_t::memory_type,
                         static_memory<relevant_types_t<basic_string<char>>>>);
      static_assert(std::is_same_v<typename string_t::traits_type,
                                   std::char_traits<char>>);
      //
      //
      return s;
      // return true; // Placeholder to avoid unused variable warning
    };
    constexpr auto s = test();
    // static_assert(s.size() == 3);
    //  static_assert(test());
    //  expect(test());
  };
};

int main(int argc, char **argv) { return 0; }
