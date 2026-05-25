// SPDX-License-Identifier: MIT

#include <boost/ut.hpp>
#include <ycetl/queue.hpp>

using namespace boost::ut;
using namespace ycetl;

suite queue_suite = [] {
  "fifo_order"_test = [] {
    constexpr auto t = [] {
      queue<int> q;
      for (int i = 1; i <= 5; ++i)
        q.push(i);
      int popped[5]{};
      int idx = 0;
      while (!q.empty()) {
        popped[idx++] = q.front();
        q.pop();
      }
      return idx == 5 && popped[0] == 1 && popped[1] == 2 && popped[2] == 3
          && popped[3] == 4 && popped[4] == 5;
    };
    static_assert(t());
    expect(t());
  };

  "front_back_peek"_test = [] {
    constexpr auto t = [] {
      queue<int> q;
      q.push(10);
      q.push(20);
      q.push(30);
      return q.size() == 3 && q.front() == 10 && q.back() == 30;
    };
    static_assert(t());
    expect(t());
  };

  "empty_then_used"_test = [] {
    constexpr auto t = [] {
      queue<int> q;
      bool was_empty = q.empty();
      q.push(7);
      bool ok = !q.empty() && q.size() == 1 && q.front() == 7;
      q.pop();
      return was_empty && ok && q.empty();
    };
    static_assert(t());
    expect(t());
  };

  "emplace"_test = [] {
    constexpr auto t = [] {
      queue<int> q;
      q.emplace(42);
      q.emplace(99);
      return q.front() == 42 && q.back() == 99;
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
