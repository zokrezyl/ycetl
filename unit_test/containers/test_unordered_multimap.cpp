// SPDX-License-Identifier: MIT

#include <boost/ut.hpp>
#include <ycetl/unordered_multimap.hpp>

using namespace boost::ut;
using namespace ycetl;

suite unordered_multimap_suite = [] {
  "insert_keeps_duplicate_keys"_test = [] {
    constexpr auto t = [] {
      unordered_multimap<int, int> m;
      m.insert({1, 10});
      m.insert({1, 20});
      m.insert({2, 200});
      m.insert({1, 30});
      return m.size() == 4 && m.count(1) == 3 && m.count(2) == 1;
    };
    static_assert(t());
    expect(t());
  };

  "find_returns_some_match"_test = [] {
    constexpr auto t = [] {
      unordered_multimap<int, int> m;
      m.insert({5, 100});
      m.insert({5, 200});
      auto it = m.find(5);
      return it != m.end() && (it->second == 100 || it->second == 200);
    };
    static_assert(t());
    expect(t());
  };

  "iterate_sees_every_pair"_test = [] {
    constexpr auto t = [] {
      unordered_multimap<int, int> m;
      for (int i = 0; i < 5; ++i)
        m.insert({7, i});
      int sum = 0, n = 0;
      for (auto &kv : m) {
        sum += kv.second;
        ++n;
      }
      return n == 5 && sum == (0 + 1 + 2 + 3 + 4);
    };
    static_assert(t());
    expect(t());
  };

  "erase_drops_all_matching_keys"_test = [] {
    constexpr auto t = [] {
      unordered_multimap<int, int> m;
      m.insert({1, 1});
      for (int i = 0; i < 3; ++i)
        m.insert({9, i});
      m.insert({2, 2});
      return m.erase(9) == 3 && m.size() == 2 && !m.contains(9) && m.contains(1)
          && m.contains(2);
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
