// SPDX-License-Identifier: MIT

/*──────────────────────────────────────────────────────────────────────────────
  test_dynamic_memory.cpp   – constexpr‑time checks
──────────────────────────────────────────────────────────────────────────────*/
#include <boost/ut.hpp>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/multitype_memory.hpp>
#include <ycetl/typed_dynamic_memory.hpp>
#include <ycetl/types.hpp>

using namespace boost::ut;
using namespace ycetl;

suite memory_suite = [] {
  /* single‑type memory, fundamental ---------------------------------- */
  "dyn_alloc_round_trip_int"_test = [] {
    constexpr auto test = [] {
      typed_dynamic_memory<int> a;
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
      typed_dynamic_memory<double> a;
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
  /* multitype memory with two POD types ------------------------------ */
  "multitype_alloc_int_double"_test = [] {
    constexpr auto test = [] {
      using types = type_set<int, double>;
      multitype_memory<typed_dynamic_memory, types> st;

      static_assert(std::is_same_v<decltype(st)::types, type_set<int, double>>);
      static_assert(std::is_same_v<decltype(st)::handled_types,
                                   type_set<typed_dynamic_memory<int>,
                                            typed_dynamic_memory<double>>>);

      auto pi = allocate<int>(st, 2);
      auto pd = allocate<double>(st, 1);

      construct_at(pi + 0); // default construct
      construct_at(pi + 1); // default construct
      construct_at(pd + 0); // default construct

      pi[0] = 7;
      pi[1] = 11;
      pd[0] = 2.0;

      bool ok = (pi[0] + pi[1] == 18 && pd[0] == 2.0_d);

      deallocate<int>(st, pi, 2);
      deallocate<double>(st, pd, 1);
      return ok;
    };
    static_assert(test());
    expect(test());
  };

  /* multitype memory with two POD types ------------------------------ */
  "multitype_alloc_int_double"_test = [] {
    constexpr auto test = [] {
      default_memory<int, double> st;

      static_assert(std::is_same_v<decltype(st)::types, type_set<int, double>>);

      auto pi = allocate<int>(st, 2);
      auto pd = allocate<double>(st, 1);

      construct_at(pi + 0); // default construct
      construct_at(pi + 1); // default construct
      construct_at(pd + 0); // default construct

      pi[0] = 7;
      pi[1] = 11;
      pd[0] = 2.0;

      bool ok = (pi[0] + pi[1] == 18 && pd[0] == 2.0_d);

      deallocate<int>(st, pi, 2);
      deallocate<double>(st, pd, 1);
      return ok;
    };
    static_assert(test());
    expect(test());
  };
};

int main(int /*argc*/, char ** /*argv*/) { return 0; }
