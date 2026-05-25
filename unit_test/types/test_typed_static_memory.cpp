// SPDX-License-Identifier: MIT

/*──────────────────────────────────────────────────────────────────────────────
  test_dynamic_memory.cpp   – constexpr‑time checks
──────────────────────────────────────────────────────────────────────────────*/
#include <boost/ut.hpp>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/typed_static_memory.hpp>
#include <ycetl/types.hpp>

using namespace boost::ut;
using namespace ycetl;

suite memory_suite = [] {
  /* single‑type memory, fundamental ---------------------------------- */
  "dyn_alloc_round_trip_int"_test = [] {
    constexpr auto test = [] {
      typed_static_memory<int> a;
      auto p = a.allocate(4);
      for (int i = 0; i < 4; ++i) {
        construct_at(p + i); // default construct
        p[i] = i + 1;        // 1 2 3 4
      }
      int sum = 0;
      for (int i = 0; i < 4; ++i)
        sum += p[i];
      a.deallocate(p, 4);
      return sum == 10_i; // 1+2+3+4
    };
    static_assert(test());
    expect(test());
  };
  "dyn_alloc_round_trip_double"_test = [] {
    constexpr auto test = [] {
      typed_static_memory<double> a;
      auto p = a.allocate(3);
      construct_at(p + 0); // default construct
      construct_at(p + 1); // default construct
      construct_at(p + 2); // default construct
      p[0] = 1.5;
      p[1] = 2.5;
      p[2] = 3.0;
      double prod = p[0] * p[1] * p[2];
      a.deallocate(p, 3);
      return prod > 10.0_d && prod < 12.0_d;
    };
    static_assert(test());
    expect(test());
  };
};

constexpr auto test_x() {
  typed_static_memory<int> a;
  return a;
}

int main(int /*argc*/, char ** /*argv*/) {
  constexpr auto sm = test_x();

  return 0;
}
