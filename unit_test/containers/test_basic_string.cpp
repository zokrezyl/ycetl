#include <boost/ut.hpp>
#include <ycetl/basic_string.hpp>
#include <ycetl/impl/typed_dynamic_memory.hpp>
#include <ycetl/memory.hpp>

using namespace boost::ut;
using ycetl::basic_string;

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
};

int main(int argc, char **argv) { return 0; }
