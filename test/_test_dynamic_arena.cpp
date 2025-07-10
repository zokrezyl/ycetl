// test_dynamic_arena.cpp
//
// Boost.UT tests for dynamic_arena<T>

#include <boost/ut.hpp>
#include <ycetl/dynamic_arena.hpp>              // dynamic_arena

using namespace boost::ut;
using ycetl::arena::dynamic_arena;

/*------------------------------------------------------------------
  dynamic_arena tests
------------------------------------------------------------------*/
suite dynamic_arena_suite = [] {

  "first_add_returns_zero"_test = [] {
    dynamic_arena<int> dp;
    expect(dp.add(42) == 0_u);
  };

  "fill_100"_test = [] {
    dynamic_arena<int> dp;
    std::size_t last{};
    for (int i = 0; i < 100; ++i) last = dp.add(i);
    expect(last == 99_u);
  };

  "fill_10000"_test = [] {
    dynamic_arena<int> dp;
    std::size_t last{};
    for (int i = 0; i < 10'000; ++i) last = dp.add(i);
    expect(last == 9'999_u);
  };
};

int main() {
  // Return success
  return 0;
}
