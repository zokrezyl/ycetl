#include <boost/ut.hpp>
#include <ycetl/impl/dynamic_memory.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/set.hpp>

using namespace boost::ut;
using ycetl::set;

suite set_suite = [] {
  "empty_set"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::set<int>>;
      using memory_t = ycetl::default_memory<relevant_types>;
      memory_t memory;
      set<int, std::less<int>, memory_t> s(memory);
      return s.empty() && s.size() == 0_u;
    };
    expect(test());
  };

  "initializer_list_construct"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::set<int>>;
      using memory_t = ycetl::default_memory<relevant_types>;
      memory_t memory;
      set<int, std::less<int>, memory_t> s({3, 1, 2}, memory);
      return s.size() == 3_u && s.contains(1) && s.contains(2) && s.contains(3);
    };
    expect(test());
  };

  "insert_elements"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::set<int>>;
      using memory_t = ycetl::default_memory<relevant_types>;
      memory_t memory;
      set<int, std::less<int>, memory_t> s(memory);
      s.insert(10);
      s.insert(5);
      s.insert(15);
      return s.size() == 3_u && s.contains(10) && s.contains(5) &&
             s.contains(15);
    };
    expect(test());
  };

  "copy_construct"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::set<int>>;
      using memory_t = ycetl::default_memory<relevant_types>;
      memory_t memory;
      set<int, std::less<int>, memory_t> a({1, 2, 3}, memory);
      set<int, std::less<int>, memory_t> b = a;
      return b.size() == 3_u && b.contains(2);
    };
    expect(test());
  };

  "move_construct"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::set<int>>;
      using memory_t = ycetl::default_memory<relevant_types>;
      memory_t memory;
      set<int, std::less<int>, memory_t> a({4, 5}, memory);
      set<int, std::less<int>, memory_t> b = std::move(a);
      return b.size() == 2_u && b.contains(4) && b.contains(5);
    };
    expect(test());
  };

  "clear_and_reuse"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::set<int>>;
      using memory_t = ycetl::default_memory<relevant_types>;
      memory_t memory;
      set<int, std::less<int>, memory_t> s({1, 2}, memory);
      s.clear();
      s.insert(42);
      return s.size() == 1_u && s.contains(42);
    };
    expect(test());
  };
};

int main(int argc, char **argv) { return 0; }
