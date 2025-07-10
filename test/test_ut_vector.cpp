// test_vector.cpp
#include <iostream>
#include <boost/ut.hpp>
#include <ycetl/vector.hpp>

using namespace boost::ut;
using ycetl::vector;

suite vector_suite = [] {
  "empty_vector"_test = [] {
    constexpr auto test = [] {
      vector<int> v;
      return v.size() == 0_u && v.capacity() == 0_u;
    };
    expect(test());
  };

  "constructed_with_capacity"_test = [] {
    constexpr auto test = [] {
      vector<int> v(8);
      return v.size() == 0_u && v.capacity() == 8_u;
    };
    expect(test());
  };

  "fill_exact_capacity"_test = [] {
    constexpr auto test = [] {
      vector<int> p(4);
      for (int i = 0; i < 4; ++i) p.push_back(i);
      return p.size() == 4_u && p[3] == 3_i;
    };
    expect(test());
  };

  "fill_100"_test = [] {
    constexpr auto test = [] {
      vector<int> p(100);
      for (int i = 0; i < 100; ++i) p.push_back(i);
      return p.size() == 100_u && p[99] == 99_i;
    };
    expect(test());
  };

  "copy_construct"_test = [] {
    constexpr auto test = [] {
      vector<int> a(3);
      a.push_back(10);
      a.push_back(20);
      a.push_back(30);
      vector<int> b = a;
      return b.size() == 3_u && b[0] == 10_i && b[2] == 30_i;
    };
    expect(test());
  };

  "move_construct"_test = [] {
    constexpr auto test = [] {
      vector<int> a(2);
      a.push_back(7);
      a.push_back(8);
      vector<int> b = std::move(a);
      return b.size() == 2_u && b[0] == 7_i && b[1] == 8_i;
    };
    expect(test());
  };

  "clear_and_reuse"_test = [] {
    constexpr auto test = [] {
      vector<int> v(5);
      v.push_back(1);
      v.push_back(2);
      v.clear();
      v.push_back(42);
      return v.size() == 1_u && v[0] == 42_i;
    };
    expect(test());
  };

  "initializer_list"_test = [] {
    constexpr auto test = [] {
      vector<int> v{1, 2, 3};
      return v.size() == 3_u && v[1] == 2_i;
    };
    expect(test());
  };

  "forward_iterator_construct"_test = [] {
    constexpr auto test = [] {
      int arr[3] = {4, 5, 6};
      vector<int> v(arr, arr + 3);
      return v.size() == 3_u && v[2] == 6_i;
    };
    expect(test());
  };

  "insert_value"_test = [] {
    constexpr auto test = [] {
      vector<int> v{1, 2, 4};
      v.insert(v.begin() + 2, 3);
      return v.size() == 4_u && v[2] == 3_i && v[3] == 4_i;
    };
    expect(test());
  };

  "emplace_back_pair"_test = [] {
    constexpr auto test = [] {
      vector<std::pair<int,int>> v;
      v.emplace_back(9, 10);
      return v.size() == 1_u && v[0].first == 9_i && v[0].second == 10_i;
    };
    expect(test());
  };
};

int main(int argc, char** argv) {}

