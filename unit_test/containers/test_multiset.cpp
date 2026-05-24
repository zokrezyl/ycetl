#include <boost/ut.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/multiset.hpp>

using namespace boost::ut;
using namespace ycetl;

suite multiset_suite = [] {
  "insert_allows_duplicates"_test = [] {
    constexpr auto t = [] {
      default_memory<int> mem;
      multiset<int> s(mem);
      s.insert(2);
      s.insert(2);
      s.insert(2);
      s.insert(1);
      s.insert(3);
      return s.size() == 5 && s.count(2) == 3 && s.count(1) == 1
          && s.count(99) == 0;
    };
    static_assert(t());
    expect(t());
  };

  "sorted_with_runs"_test = [] {
    constexpr auto t = [] {
      default_memory<int> mem;
      multiset<int> s(mem);
      for (int k : {5, 1, 5, 3, 1, 5})
        s.insert(k);
      int prev = -1;
      bool ok = true;
      for (auto k : s) {
        ok = ok && k >= prev;
        prev = k;
      }
      return ok && s.count(5) == 3 && s.count(1) == 2 && s.count(3) == 1;
    };
    static_assert(t());
    expect(t());
  };

  "equal_range_brackets_run"_test = [] {
    constexpr auto t = [] {
      default_memory<int> mem;
      multiset<int> s(mem);
      for (int k : {1, 2, 2, 2, 3})
        s.insert(k);
      auto [lo, hi] = s.equal_range(2);
      int seen = 0;
      for (auto it = lo; it != hi; ++it) {
        ++seen;
        if (*it != 2)
          return false;
      }
      return seen == 3;
    };
    static_assert(t());
    expect(t());
  };

  "erase_removes_all_matching"_test = [] {
    constexpr auto t = [] {
      default_memory<int> mem;
      multiset<int> s(mem);
      for (int k : {1, 2, 2, 2, 3})
        s.insert(k);
      bool ok = s.erase(2) == 3 && s.size() == 2 && !s.contains(2)
             && s.contains(1) && s.contains(3);
      return ok && s.erase(999) == 0;
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
