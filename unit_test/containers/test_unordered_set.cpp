// SPDX-License-Identifier: MIT

#include <boost/ut.hpp>
#include <ycetl/unordered_set.hpp>

using namespace boost::ut;
using namespace ycetl;

suite unordered_set_suite = [] {
  "insert_and_contains"_test = [] {
    constexpr auto t = [] {
      unordered_set<int> s;
      s.insert(1);
      s.insert(2);
      s.insert(3);
      return s.size() == 3 && s.contains(2) && s.contains(3) && !s.contains(99)
          && !s.empty();
    };
    static_assert(t());
    expect(t());
  };

  "duplicate_insert_no_op"_test = [] {
    constexpr auto t = [] {
      unordered_set<int> s;
      auto [it1, ins1] = s.insert(42);
      auto [it2, ins2] = s.insert(42);
      return ins1 && !ins2 && s.size() == 1;
    };
    static_assert(t());
    expect(t());
  };

  "erase_tombstone"_test = [] {
    // Tombstoning the middle of a probe chain must keep the rest of
    // the chain findable.
    constexpr auto t = [] {
      unordered_set<int> s;
      for (int i = 0; i < 20; ++i)
        s.insert(i);
      bool ok = s.erase(7) == 1 && s.erase(13) == 1;
      for (int i = 0; i < 20; ++i) {
        if (i == 7 || i == 13)
          ok = ok && !s.contains(i);
        else
          ok = ok && s.contains(i);
      }
      return ok && s.erase(999) == 0 && s.size() == 18;
    };
    static_assert(t());
    expect(t());
  };

  "rehash_under_growth"_test = [] {
    constexpr auto t = [] {
      unordered_set<int> s;
      for (int i = 0; i < 100; ++i)
        s.insert(i);
      bool ok = s.size() == 100 && s.bucket_count() >= 128;
      for (int i = 0; i < 100; ++i)
        ok = ok && s.contains(i);
      return ok;
    };
    static_assert(t());
    expect(t());
  };

  "iterate"_test = [] {
    constexpr auto t = [] {
      unordered_set<int> s;
      for (int i = 1; i <= 5; ++i)
        s.insert(i);
      int sum = 0;
      for (auto v : s)
        sum += v;
      return sum == 15;
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
