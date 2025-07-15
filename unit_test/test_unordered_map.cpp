#include <boost/ut.hpp>
#include <ycetl/impl/dynamic_allocator.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/unordered_map.hpp>

using namespace boost::ut;
using ycetl::unordered_map;

suite unordered_map_suite = [] {
  "empty_unordered_map"_test = [] {
    constexpr auto test = [] {
      using relevant_types =
          ycetl::relevant_types_t<ycetl::unordered_map<int, int>>;
      using allocator_t = ycetl::default_allocator<relevant_types>;
      allocator_t allocator;
      unordered_map<int, int, std::hash<int>, std::equal_to<int>, allocator_t>
          m(allocator);
      return m.empty() && m.size() == 0_u;
    };
    expect(test());
  };

  "insert_and_find"_test = [] {
    constexpr auto test = [] {
      using relevant_types =
          ycetl::relevant_types_t<ycetl::unordered_map<int, int>>;
      using allocator_t = ycetl::default_allocator<relevant_types>;
      allocator_t allocator;
      unordered_map<int, int, std::hash<int>, std::equal_to<int>, allocator_t>
          m(allocator);
      m.insert({1, 10});
      m.insert({2, 20});
      auto it = m.find(2);
      return m.size() == 2_u && it != m.end() && it->second == 20_i;
    };
    expect(test());
  };

  "bracket_operator"_test = [] {
    constexpr auto test = [] {
      using relevant_types =
          ycetl::relevant_types_t<ycetl::unordered_map<int, int>>;
      using allocator_t = ycetl::default_allocator<relevant_types>;
      allocator_t allocator;
      unordered_map<int, int, std::hash<int>, std::equal_to<int>, allocator_t>
          m(allocator);
      m[3] = 30;
      return m.size() == 1_u && m[3] == 30_i;
    };
    expect(test());
  };

  "copy_construct"_test = [] {
    constexpr auto test = [] {
      using relevant_types =
          ycetl::relevant_types_t<ycetl::unordered_map<int, int>>;
      using allocator_t = ycetl::default_allocator<relevant_types>;
      allocator_t allocator;
      unordered_map<int, int, std::hash<int>, std::equal_to<int>, allocator_t>
          a(allocator);
      a.insert({4, 40});
      unordered_map<int, int, std::hash<int>, std::equal_to<int>, allocator_t>
          b = a;
      return b.size() == 1_u && b[4] == 40_i;
    };
    expect(test());
  };

  "move_construct"_test = [] {
    constexpr auto test = [] {
      using relevant_types =
          ycetl::relevant_types_t<ycetl::unordered_map<int, int>>;
      using allocator_t = ycetl::default_allocator<relevant_types>;
      allocator_t allocator;
      unordered_map<int, int, std::hash<int>, std::equal_to<int>, allocator_t>
          a(allocator);
      a.insert({5, 50});
      unordered_map<int, int, std::hash<int>, std::equal_to<int>, allocator_t>
          b = std::move(a);
      return b.size() == 1_u && b[5] == 50_i;
    };
    expect(test());
  };

  "clear_and_reuse"_test = [] {
    constexpr auto test = [] {
      using relevant_types =
          ycetl::relevant_types_t<ycetl::unordered_map<int, int>>;
      using allocator_t = ycetl::default_allocator<relevant_types>;
      allocator_t allocator;
      unordered_map<int, int, std::hash<int>, std::equal_to<int>, allocator_t>
          m(allocator);
      m.insert({1, 10});
      m.clear();
      m.insert({2, 20});
      return m.size() == 1_u && m[2] == 20_i && !m.contains(1);
    };
    expect(test());
  };
};

int main(int argc, char **argv) { return 0; }
