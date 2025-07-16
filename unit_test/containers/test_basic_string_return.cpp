#include <boost/ut.hpp>
#include <ycetl/basic_string.hpp>
#include <ycetl/memory.hpp>

using namespace boost::ut;
using ycetl::basic_string;

// First constexpr function creates and returns a string
constexpr auto get_string() {
  basic_string<char> s("hello");
  return s; // triggers copy of allocations
}

// Second constexpr function receives, modifies, and returns the string
constexpr auto add_to_string() {
  auto s = get_string();
  s.push_back(' ');
  s.push_back('w');
  s.push_back('o');
  s.push_back('r');
  s.push_back('l');
  s.push_back('d');
  return s; // another implicit memory copy here
};

suite nested_string_suite = [] {
  "nested_string_constexpr_test"_test = [] {
    constexpr auto test = [] {
      auto s = add_to_string();
      bool size_correct = (s.size() == 11_u);
      bool content_correct = (s[0] == 'h' && s[10] == 'd');
      bool memory_allocated = (s.capacity() >= s.size()); // memory correctness
      return size_correct && content_correct && memory_allocated;
    };

    static_assert(test());
    expect(test());
  };

  "nested_string_multiple_modifications"_test = [] {
    constexpr auto test = [] {
      auto s = add_to_string();
      s.push_back('!');
      s.push_back('?');
      bool size_correct = (s.size() == 13_u);
      bool last_chars_correct = (s[11] == '!' && s[12] == '?');
      bool memory_correct = (s.capacity() >= s.size());
      return size_correct && last_chars_correct && memory_correct;
    };

    static_assert(test());
    expect(test());
  };

  "nested_string_clear_and_reuse"_test = [] {
    constexpr auto test = [] {
      auto s = add_to_string();
      s.clear();
      s.push_back('x');
      bool clear_correct = (s.size() == 1_u && s[0] == 'x');
      bool memory_correct = (s.capacity() >= s.size());
      return clear_correct && memory_correct;
    };

    static_assert(test());
    expect(test());
  };
};

int main(int argc, char **argv) { return 0; }
