#include <boost/ut.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/set.hpp>

using namespace boost::ut;
using namespace ycetl;

suite set_suite = [] {
  "insert_and_contains"_test = [] {
    constexpr auto t = [] {
      default_memory<int> mem;
      set<int> s(mem);
      s.insert(5);
      s.insert(2);
      s.insert(8);
      return s.size() == 3 && s.contains(2) && s.contains(8) && !s.contains(99);
    };
    static_assert(t());
    expect(t());
  };

  "duplicate_insert_no_op"_test = [] {
    constexpr auto t = [] {
      default_memory<int> mem;
      set<int> s(mem);
      auto [it1, ins1] = s.insert(42);
      auto [it2, ins2] = s.insert(42);
      return ins1 && !ins2 && s.size() == 1;
    };
    static_assert(t());
    expect(t());
  };

  "iterates_in_sorted_order"_test = [] {
    constexpr auto t = [] {
      default_memory<int> mem;
      set<int> s(mem);
      for (int k : {5, 2, 8, 1, 9, 3})
        s.insert(k);
      int prev = -1;
      bool ok = true;
      for (auto &k : s) {
        ok = ok && k > prev;
        prev = k;
      }
      return ok && prev == 9 && s.size() == 6;
    };
    static_assert(t());
    expect(t());
  };

  "erase_preserves_order"_test = [] {
    constexpr auto t = [] {
      default_memory<int> mem;
      set<int> s(mem);
      for (int k : {1, 2, 3, 4, 5})
        s.insert(k);
      bool ok = s.erase(3) == 1 && s.size() == 4 && !s.contains(3);
      int prev = 0;
      for (auto &k : s) {
        ok = ok && k > prev;
        prev = k;
      }
      return ok && s.erase(999) == 0;
    };
    static_assert(t());
    expect(t());
  };

  "find_returns_iterator"_test = [] {
    constexpr auto t = [] {
      default_memory<int> mem;
      set<int> s(mem);
      s.insert(10);
      s.insert(20);
      auto it = s.find(20);
      return it != s.end() && *it == 20 && s.find(99) == s.end();
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
