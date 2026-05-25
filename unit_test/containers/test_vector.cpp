// SPDX-License-Identifier: MIT

#include <boost/ut.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/vector.hpp>

using namespace boost::ut;
using namespace ycetl;

suite vector_suite = [] {
  "push_back_and_size"_test = [] {
    constexpr auto t = [] {
      default_memory<int> mem;
      vector<int> v(mem);
      for (int i = 1; i <= 5; ++i)
        v.push_back(i);
      return v.size() == 5 && v.front() == 1 && v.back() == 5 && v[2] == 3;
    };
    static_assert(t());
    expect(t());
  };

  "default_ctor_uses_typed_dynamic_memory"_test = [] {
    constexpr auto t = [] {
      vector<int> v; // no Memory argument
      v.push_back(10);
      v.push_back(20);
      return v.size() == 2 && v[0] == 10 && v[1] == 20;
    };
    static_assert(t());
    expect(t());
  };

  "reserve_grows_capacity_without_size"_test = [] {
    constexpr auto t = [] {
      vector<int> v;
      v.reserve(16);
      return v.capacity() >= 16 && v.size() == 0;
    };
    static_assert(t());
    expect(t());
  };

  "resize_with_value"_test = [] {
    constexpr auto t = [] {
      vector<int> v;
      v.resize(4, 7);
      int sum = 0;
      for (auto x : v)
        sum += x;
      return v.size() == 4 && sum == 28;
    };
    static_assert(t());
    expect(t());
  };

  "iterate_and_modify"_test = [] {
    constexpr auto t = [] {
      vector<int> v;
      for (int i = 0; i < 5; ++i)
        v.push_back(i);
      for (auto &x : v)
        x *= 10;
      return v[0] == 0 && v[4] == 40;
    };
    static_assert(t());
    expect(t());
  };

  "pop_back_shrinks"_test = [] {
    constexpr auto t = [] {
      vector<int> v;
      v.push_back(1);
      v.push_back(2);
      v.push_back(3);
      v.pop_back();
      return v.size() == 2 && v.back() == 2;
    };
    static_assert(t());
    expect(t());
  };

  "equality"_test = [] {
    constexpr auto t = [] {
      vector<int> a, b;
      for (int i = 0; i < 3; ++i) {
        a.push_back(i);
        b.push_back(i);
      }
      bool eq = (a == b);
      b.push_back(99);
      return eq && !(a == b);
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
