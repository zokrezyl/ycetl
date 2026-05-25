// SPDX-License-Identifier: MIT

#include <boost/ut.hpp>
#include <ycetl/unordered_map.hpp>

using namespace boost::ut;
using namespace ycetl;

suite unordered_map_suite = [] {
  "insert_and_lookup"_test = [] {
    constexpr auto t = [] {
      unordered_map<int, int> m;
      m.insert({1, 100});
      m.insert({2, 200});
      m.insert({3, 300});
      return m.size() == 3 && m.contains(2) && !m.contains(99)
          && m.find(3)->second == 300 && m.find(99) == m.end();
    };
    static_assert(t());
    expect(t());
  };

  "operator_bracket_default_inserts"_test = [] {
    constexpr auto t = [] {
      unordered_map<int, int> m;
      int &v = m[42];
      bool ok = m.size() == 1 && v == 0;
      m[42] = 7;
      return ok && m[42] == 7 && m.size() == 1;
    };
    static_assert(t());
    expect(t());
  };

  "insert_or_assign"_test = [] {
    constexpr auto t = [] {
      unordered_map<int, int> m;
      auto [it1, ins1] = m.insert_or_assign(1, 10);
      auto [it2, ins2] = m.insert_or_assign(1, 99);
      return ins1 && !ins2 && m.size() == 1 && m[1] == 99;
    };
    static_assert(t());
    expect(t());
  };

  "erase_and_tombstone"_test = [] {
    // Tombstones must not block subsequent lookups of other keys
    // probed through the same bucket — open-addressing's signature
    // test.
    constexpr auto t = [] {
      unordered_map<int, int> m;
      for (int i = 0; i < 20; ++i)
        m.insert({i, i * 10});
      bool ok = m.size() == 20;
      ok = ok && m.erase(7) == 1 && m.erase(13) == 1;
      ok = ok && m.size() == 18 && !m.contains(7) && !m.contains(13);
      // Every other key still lookable
      for (int i = 0; i < 20; ++i) {
        if (i == 7 || i == 13)
          continue;
        ok = ok && m.find(i) != m.end() && m.find(i)->second == i * 10;
      }
      return ok && m.erase(999) == 0;
    };
    static_assert(t());
    expect(t());
  };

  "rehash_under_growth"_test = [] {
    // Crossing 70% of the initial 8-slot table at 6 inserts forces
    // a rehash to 16. Beyond that we keep doubling. Verifies entries
    // survive every rehash.
    constexpr auto t = [] {
      unordered_map<int, int> m;
      for (int i = 0; i < 100; ++i)
        m.insert({i, i + 1000});
      bool ok = m.size() == 100 && m.bucket_count() >= 128;
      for (int i = 0; i < 100; ++i)
        ok = ok && m.find(i) != m.end() && m.find(i)->second == i + 1000;
      return ok;
    };
    static_assert(t());
    expect(t());
  };

  "iterate_skips_tombstones_and_empties"_test = [] {
    constexpr auto t = [] {
      unordered_map<int, int> m;
      for (int i = 0; i < 10; ++i)
        m.insert({i, i});
      m.erase(3);
      m.erase(7);
      int count = 0, sum = 0;
      for (auto &kv : m) {
        ++count;
        sum += kv.first;
      }
      return count == 8 && sum == (0 + 1 + 2 + 4 + 5 + 6 + 8 + 9);
    };
    static_assert(t());
    expect(t());
  };

  "duplicate_insert_no_op"_test = [] {
    constexpr auto t = [] {
      unordered_map<int, int> m;
      auto [it1, ins1] = m.insert({5, 50});
      auto [it2, ins2] = m.insert({5, 999});
      return ins1 && !ins2 && m.size() == 1 && m[5] == 50;
    };
    static_assert(t());
    expect(t());
  };

  "clear"_test = [] {
    constexpr auto t = [] {
      unordered_map<int, int> m;
      for (int i = 0; i < 5; ++i)
        m.insert({i, i});
      m.clear();
      bool ok = m.empty() && m.size() == 0;
      m.insert({42, 100});
      return ok && m.size() == 1 && m[42] == 100;
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
