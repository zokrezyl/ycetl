#include <boost/ut.hpp>
#include <ycetl/impl/dynamic_allocator.hpp>
#include <ycetl/list.hpp>
#include <ycetl/memory.hpp>

using namespace boost::ut;
using ycetl::list;

struct Test {
  int value;
  constexpr Test(int v = 0) : value(v) {}
};

suite list_suite = [] {
  "empty_list"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::list<int>>;
      using allocator_t = ycetl::default_allocator<relevant_types>;
      allocator_t allocator;
      list<int, allocator_t> l(allocator);
      return l.empty() && l.size() == 0_u;
    };
    expect(test());
  };

  "list_with_values"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::list<int>>;
      using allocator_t = ycetl::default_allocator<relevant_types>;
      allocator_t allocator;
      list<int, allocator_t> l({1, 2, 3}, allocator);
      return l.size() == 3_u && l.front() == 1_i && l.back() == 3_i;
    };
    expect(test());
  };

  "push_back_and_front"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::list<int>>;
      using allocator_t = ycetl::default_allocator<relevant_types>;
      allocator_t allocator;
      list<int, allocator_t> l(allocator);
      l.push_back(10);
      l.push_front(5);
      return l.size() == 2_u && l.front() == 5_i && l.back() == 10_i;
    };
    expect(test());
  };

  "emplace_back"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::list<Test>>;
      using allocator_t = ycetl::default_allocator<relevant_types>;
      allocator_t allocator;
      list<Test, allocator_t> l(allocator);
      l.emplace_back(42);
      return l.size() == 1_u && l.back().value == 42_i;
    };
    expect(test());
  };

  "copy_construct"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::list<int>>;
      using allocator_t = ycetl::default_allocator<relevant_types>;
      allocator_t allocator;
      list<int, allocator_t> a({1, 2, 3}, allocator);
      list<int, allocator_t> b = a;
      return b.size() == 3_u && b.front() == 1_i && b.back() == 3_i;
    };
    expect(test());
  };

  "move_construct"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::list<int>>;
      using allocator_t = ycetl::default_allocator<relevant_types>;
      allocator_t allocator;
      list<int, allocator_t> a({7, 8}, allocator);
      list<int, allocator_t> b = std::move(a);
      return b.size() == 2_u && b.front() == 7_i && b.back() == 8_i;
    };
    expect(test());
  };

  "clear_and_reuse"_test = [] {
    constexpr auto test = [] {
      using relevant_types = ycetl::relevant_types_t<ycetl::list<int>>;
      using allocator_t = ycetl::default_allocator<relevant_types>;
      allocator_t allocator;
      list<int, allocator_t> l({1, 2}, allocator);
      l.clear();
      l.push_back(42);
      return l.size() == 1_u && l.front() == 42_i;
    };
    expect(test());
  };
};

int main(int argc, char **argv) { return 0; }
