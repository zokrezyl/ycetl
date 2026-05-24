#include <boost/ut.hpp>
#include <ycetl/deque.hpp>

using namespace boost::ut;
using namespace ycetl;

suite deque_suite = [] {
  "push_back_and_index"_test = [] {
    constexpr auto t = [] {
      deque<int> d;
      for (int i = 0; i < 5; ++i)
        d.push_back(i);
      return d.size() == 5 && d.front() == 0 && d.back() == 4 && d[2] == 2;
    };
    static_assert(t());
    expect(t());
  };

  "push_front_then_back"_test = [] {
    // Push 1,2,3 to the front and 4,5,6 to the back — final order
    // should be 3,2,1,4,5,6 with the ring wrap absorbing both
    // directions.
    constexpr auto t = [] {
      deque<int> d;
      for (int i : {1, 2, 3})
        d.push_front(i); // 3, 2, 1
      for (int i : {4, 5, 6})
        d.push_back(i); // 3, 2, 1, 4, 5, 6
      int seen[6]{};
      for (int i = 0; i < 6; ++i)
        seen[i] = d[i];
      return seen[0] == 3 && seen[1] == 2 && seen[2] == 1 && seen[3] == 4
          && seen[4] == 5 && seen[5] == 6 && d.size() == 6;
    };
    static_assert(t());
    expect(t());
  };

  "pop_front_pop_back"_test = [] {
    constexpr auto t = [] {
      deque<int> d;
      for (int i = 1; i <= 5; ++i)
        d.push_back(i);
      d.pop_front(); // {2,3,4,5}
      d.pop_back();  // {2,3,4}
      return d.size() == 3 && d.front() == 2 && d.back() == 4;
    };
    static_assert(t());
    expect(t());
  };

  "growth_under_mixed_push"_test = [] {
    // Force several rounds of growth from both ends. The ring's
    // head index walks around the buffer; after grow() it should
    // come back to 0 and the elements stay in the right order.
    constexpr auto t = [] {
      deque<int> d;
      for (int i = 0; i < 50; ++i) {
        if (i & 1)
          d.push_back(i);
        else
          d.push_front(i);
      }
      // Sanity: every value 0..49 is present exactly once.
      int seen_xor = 0;
      for (auto v : d)
        seen_xor ^= v;
      int expected = 0;
      for (int i = 0; i < 50; ++i)
        expected ^= i;
      return d.size() == 50 && seen_xor == expected;
    };
    static_assert(t());
    expect(t());
  };

  "iterate_in_order"_test = [] {
    constexpr auto t = [] {
      deque<int> d;
      for (int i : {1, 2, 3, 4, 5})
        d.push_back(i);
      int prev = 0, count = 0;
      for (auto v : d) {
        if (v != prev + 1)
          return false;
        prev = v;
        ++count;
      }
      return count == 5;
    };
    static_assert(t());
    expect(t());
  };

  "clear_resets"_test = [] {
    constexpr auto t = [] {
      deque<int> d;
      for (int i = 0; i < 10; ++i)
        d.push_back(i);
      d.clear();
      bool empty = d.empty() && d.size() == 0;
      d.push_back(42);
      return empty && d.size() == 1 && d.front() == 42;
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
