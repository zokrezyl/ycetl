#include <boost/ut.hpp>
#include <ycetl/impl/dynamic_memory.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/string.hpp>

using namespace boost::ut;
using ycetl::string;

suite string_suite = [] {
  "empty_string"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::string<char>>;
      using memory_t = ycetl::default_memory<relevant_types>;
      memory_t memory;
      string<char, std::char_traits<char>, memory_t> s(memory);
      return s.empty() && s.size() == 0_u;
    };
    expect(test());
  };

  "construct_from_literal"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::string<char>>;
      using memory_t = ycetl::default_memory<relevant_types>;
      memory_t memory;
      string<char, std::char_traits<char>, memory_t> s("hello", memory);
      return s.size() == 5_u && s[0] == 'h' && s[4] == 'o';
    };
    expect(test());
  };

  "push_back"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::string<char>>;
      using memory_t = ycetl::default_memory<relevant_types>;
      memory_t memory;
      string<char, std::char_traits<char>, memory_t> s(memory);
      s.push_back('x');
      s.push_back('y');
      return s.size() == 2_u && s[0] == 'x' && s[1] == 'y';
    };
    expect(test());
  };

  "copy_construct"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::string<char>>;
      using memory_t = ycetl::default_memory<relevant_types>;
      memory_t memory;
      string<char, std::char_traits<char>, memory_t> a("abc", memory);
      string<char, std::char_traits<char>, memory_t> b = a;
      return b.size() == 3_u && b[0] == 'a' && b[2] == 'c';
    };
    expect(test());
  };

  "move_construct"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::string<char>>;
      using memory_t = ycetl::default_memory<relevant_types>;
      memory_t memory;
      string<char, std::char_traits<char>, memory_t> a("xy", memory);
      string<char, std::char_traits<char>, memory_t> b = std::move(a);
      return b.size() == 2_u && b[0] == 'x' && b[1] == 'y';
    };
    expect(test());
  };

  "clear_and_reuse"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::string<char>>;
      using memory_t = ycetl::default_memory<relevant_types>;
      memory_t memory;
      string<char, std::char_traits<char>, memory_t> s("test", memory);
      s.clear();
      s.push_back('z');
      return s.size() == 1_u && s[0] == 'z';
    };
    expect(test());
  };

  "initializer_list"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::string<char>>;
      using memory_t = ycetl::default_memory<relevant_types>;
      memory_t memory;
      string<char, std::char_traits<char>, memory_t> s({'a', 'b', 'c'}, memory);
      return s.size() == 3_u && s[1] == 'b';
    };
    expect(test());
  };
};

int main(int argc, char **argv) { return 0; }
