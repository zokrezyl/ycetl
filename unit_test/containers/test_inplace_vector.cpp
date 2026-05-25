// SPDX-License-Identifier: MIT

#include <boost/ut.hpp>
#include <ycetl/inplace_vector.hpp>

using namespace boost::ut;
using namespace ycetl;

suite inplace_vector_suite = [] {
  "push_back_and_size"_test = [] {
    constexpr auto t = [] {
      inplace_vector<int, 16> v;
      for (int i = 1; i <= 5; ++i)
        v.push_back(i);
      return v.size() == 5 && v.front() == 1 && v.back() == 5 && v[2] == 3;
    };
    static_assert(t());
    expect(t());
  };

  "capacity_is_baked_into_type"_test = [] {
    constexpr auto t = [] {
      inplace_vector<int, 8> v;
      return inplace_vector<int, 8>::capacity() == 8 && v.empty() && !v.full();
    };
    static_assert(t());
    expect(t());
  };

  "initializer_list_construction"_test = [] {
    constexpr auto t = [] {
      inplace_vector<int, 4> v{1, 2, 3};
      return v.size() == 3 && v[0] == 1 && v[2] == 3;
    };
    static_assert(t());
    expect(t());
  };

  "fill_to_capacity_marks_full"_test = [] {
    constexpr auto t = [] {
      inplace_vector<int, 5> v;
      for (int i = 0; i < 5; ++i)
        v.push_back(i);
      return v.size() == 5 && v.full() && v[4] == 4;
    };
    static_assert(t());
    expect(t());
  };

  "pop_back_shrinks"_test = [] {
    constexpr auto t = [] {
      inplace_vector<int, 8> v{10, 20, 30};
      v.pop_back();
      return v.size() == 2 && v.back() == 20;
    };
    static_assert(t());
    expect(t());
  };

  "resize_grows_with_value"_test = [] {
    constexpr auto t = [] {
      inplace_vector<int, 8> v;
      v.resize(4, 7);
      int sum = 0;
      for (auto x : v)
        sum += x;
      return v.size() == 4 && sum == 28;
    };
    static_assert(t());
    expect(t());
  };

  "resize_shrinks"_test = [] {
    constexpr auto t = [] {
      inplace_vector<int, 8> v{1, 2, 3, 4, 5};
      v.resize(2);
      return v.size() == 2 && v[0] == 1 && v[1] == 2;
    };
    static_assert(t());
    expect(t());
  };

  "clear_empties"_test = [] {
    constexpr auto t = [] {
      inplace_vector<int, 8> v{1, 2, 3};
      v.clear();
      return v.empty();
    };
    static_assert(t());
    expect(t());
  };

  "iterate_and_modify"_test = [] {
    constexpr auto t = [] {
      inplace_vector<int, 8> v{1, 2, 3, 4};
      for (auto &x : v)
        x *= 10;
      return v.size() == 4 && v[0] == 10 && v[3] == 40;
    };
    static_assert(t());
    expect(t());
  };

  "equality"_test = [] {
    constexpr auto t = [] {
      inplace_vector<int, 8> a{1, 2, 3};
      inplace_vector<int, 8> b{1, 2, 3};
      inplace_vector<int, 8> c{1, 2};
      return (a == b) && !(a == c);
    };
    static_assert(t());
    expect(t());
  };

  // The point of the type: it survives a constexpr build into a runtime
  // constant living in .rodata, with no heap involved.
  "ships_as_result_memory"_test = [] {
    constexpr auto build = [] {
      inplace_vector<int, 8> v;
      for (int i = 1; i <= 4; ++i)
        v.push_back(i * i);
      return v;
    };
    static constexpr auto baked = build();
    static_assert(baked.size() == 4);
    static_assert(baked[0] == 1);
    static_assert(baked[3] == 16);
    expect(baked.size() == 4_u);
    expect(baked.back() == 16);
  };

  "emplace_back_returns_reference"_test = [] {
    constexpr auto t = [] {
      inplace_vector<int, 4> v;
      auto &r = v.emplace_back(42);
      r = 99;
      return v.size() == 1 && v[0] == 99;
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
