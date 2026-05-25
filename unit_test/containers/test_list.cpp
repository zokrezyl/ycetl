// SPDX-License-Identifier: MIT

#include <boost/ut.hpp>
#include <ycetl/list.hpp>

using namespace boost::ut;
using namespace ycetl;

suite list_suite = [] {
  "push_back_and_iterate"_test = [] {
    constexpr auto t = [] {
      list<int> l;
      for (int i = 1; i <= 5; ++i)
        l.push_back(i);
      int sum = 0;
      for (auto v : l)
        sum += v;
      return l.size() == 5 && sum == 15 && l.front() == 1 && l.back() == 5;
    };
    static_assert(t());
    expect(t());
  };

  "push_front_reverses_order"_test = [] {
    constexpr auto t = [] {
      list<int> l;
      for (int i = 1; i <= 4; ++i)
        l.push_front(i); // 4, 3, 2, 1
      return l.front() == 4 && l.back() == 1 && l.size() == 4;
    };
    static_assert(t());
    expect(t());
  };

  "bidirectional_iteration"_test = [] {
    constexpr auto t = [] {
      list<int> l{10, 20, 30, 40};
      auto it = l.end();
      --it;
      bool ok = *it == 40;
      --it;
      ok = ok && *it == 30;
      --it;
      ok = ok && *it == 20;
      return ok;
    };
    static_assert(t());
    expect(t());
  };

  "insert_in_middle"_test = [] {
    constexpr auto t = [] {
      list<int> l{1, 2, 4, 5};
      auto it = l.begin();
      ++it;
      ++it; // points at 4
      l.insert(it, 3);
      int prev = 0;
      for (auto v : l) {
        if (v != prev + 1)
          return false;
        prev = v;
      }
      return prev == 5 && l.size() == 5;
    };
    static_assert(t());
    expect(t());
  };

  "erase_returns_next"_test = [] {
    constexpr auto t = [] {
      list<int> l{1, 2, 3, 4};
      auto it = l.begin();
      ++it; // points at 2
      it = l.erase(it);
      return *it == 3 && l.size() == 3 && l.front() == 1 && l.back() == 4;
    };
    static_assert(t());
    expect(t());
  };

  "pop_front_pop_back"_test = [] {
    constexpr auto t = [] {
      list<int> l{1, 2, 3};
      l.pop_front();
      l.pop_back();
      return l.size() == 1 && l.front() == 2 && l.back() == 2;
    };
    static_assert(t());
    expect(t());
  };

  "copy_then_mutate_independently"_test = [] {
    constexpr auto t = [] {
      list<int> a{1, 2, 3};
      list<int> b = a;
      b.push_back(4);
      return a.size() == 3 && b.size() == 4 && a.back() == 3 && b.back() == 4;
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
