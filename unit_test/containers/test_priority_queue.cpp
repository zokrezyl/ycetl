// SPDX-License-Identifier: MIT

#include <boost/ut.hpp>
#include <functional>
#include <ycetl/memory.hpp>
#include <ycetl/priority_queue.hpp>

using namespace boost::ut;
using namespace ycetl;

suite priority_queue_suite = [] {
  "max_heap_default"_test = [] {
    // Default Compare = std::less → top() is the largest.
    constexpr auto t = [] {
      default_memory<int> mem;
      priority_queue<int> pq(mem);
      for (int v : {3, 1, 4, 1, 5, 9, 2, 6})
        pq.push(v);

      int prev = 100;
      bool ok = pq.size() == 8;
      while (!pq.empty()) {
        int top = pq.top();
        ok = ok && (top <= prev);
        prev = top;
        pq.pop();
      }
      return ok;
    };
    static_assert(t());
    expect(t());
  };

  "min_heap_with_greater"_test = [] {
    constexpr auto t = [] {
      default_memory<int> mem;
      priority_queue<int, dynamic_array<int>, std::greater<int>> pq(mem);
      for (int v : {5, 2, 8, 1, 9, 3})
        pq.push(v);
      int prev = -1;
      bool ok = true;
      while (!pq.empty()) {
        int top = pq.top();
        ok = ok && (top >= prev);
        prev = top;
        pq.pop();
      }
      return ok && prev == 9;
    };
    static_assert(t());
    expect(t());
  };

  "single_element"_test = [] {
    constexpr auto t = [] {
      default_memory<int> mem;
      priority_queue<int> pq(mem);
      pq.push(42);
      bool ok = pq.size() == 1 && pq.top() == 42;
      pq.pop();
      return ok && pq.empty();
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
