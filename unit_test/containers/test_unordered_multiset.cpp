#include <boost/ut.hpp>
#include <ycetl/unordered_multiset.hpp>

using namespace boost::ut;
using namespace ycetl;

suite unordered_multiset_suite = [] {
  "insert_keeps_duplicates"_test = [] {
    constexpr auto t = [] {
      unordered_multiset<int> s;
      s.insert(7);
      s.insert(7);
      s.insert(7);
      s.insert(1);
      return s.size() == 4 && s.count(7) == 3 && s.count(1) == 1
          && s.count(99) == 0 && s.contains(7);
    };
    static_assert(t());
    expect(t());
  };

  "count_walks_full_probe_chain"_test = [] {
    // 20 distinct keys + 5 copies of one key spread across rehashes
    // — count must collect every copy regardless of probe-chain
    // length.
    constexpr auto t = [] {
      unordered_multiset<int> s;
      for (int i = 0; i < 20; ++i)
        s.insert(i);
      for (int i = 0; i < 5; ++i)
        s.insert(42);
      return s.count(42) == 5 && s.size() == 25;
    };
    static_assert(t());
    expect(t());
  };

  "erase_removes_all_matching"_test = [] {
    constexpr auto t = [] {
      unordered_multiset<int> s;
      s.insert(1);
      for (int i = 0; i < 4; ++i)
        s.insert(7);
      s.insert(3);
      return s.erase(7) == 4 && s.size() == 2 && !s.contains(7) && s.contains(1)
          && s.contains(3) && s.erase(999) == 0;
    };
    static_assert(t());
    expect(t());
  };

  "rehash_preserves_duplicates"_test = [] {
    constexpr auto t = [] {
      unordered_multiset<int> s;
      for (int i = 0; i < 50; ++i) {
        s.insert(5);
        s.insert(i);
      }
      return s.size() == 100
          && s.count(5) == 51 // 50 copies + key=5 from the i-loop
          && s.bucket_count() >= 128;
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
