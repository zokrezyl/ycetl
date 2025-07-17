/*──────────────────────────────────────────────────────────────────────────────
  test_dynamic_memory.cpp   – constexpr‑time checks
──────────────────────────────────────────────────────────────────────────────*/
#include <boost/ut.hpp>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/impl/dynamic_memory.hpp>
#include <ycetl/impl/multitype_memory.hpp>
#include <ycetl/types.hpp>

using namespace boost::ut;
namespace mem = ycetl::memory;

/* shorthand for a simple dynamic_memory */
template <class T> using dyn_alloc = mem::dynamic_memory<T>;
/*──────────────────────────────────────────────────────────────────────────────
  Extra checks for dynamic_memory & multitype_memory (constexpr only)
──────────────────────────────────────────────────────────────────────────────*/

suite memory_suite = [] {
  /* single‑type memory, fundamental ---------------------------------- */
  "dyn_alloc_round_trip_int"_test = [] {
    constexpr auto test = [] {
      ycetl::memory::dynamic_memory<int> a;
      int *p = a.allocate(4);
      for (int i = 0; i < 4; ++i) {
        std::construct_at(p + i); // default construct
        p[i] = i + 1;             // 1 2 3 4
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
      ycetl::memory::dynamic_memory<double> a;
      double *p = a.allocate(3);
      std::construct_at(p + 0); // default construct
      std::construct_at(p + 1); // default construct
      std::construct_at(p + 2); // default construct
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
      using types = ycetl::type_set<int, double>;
      ycetl::memory::multitype_memory<ycetl::memory::dynamic_memory, types> st;

      int *pi = st.template allocate<int>(2);
      double *pd = st.template allocate<double>(1);
      std::construct_at(pi + 0); // default construct
      std::construct_at(pi + 1); // default construct
      std::construct_at(pd + 0); // default construct

      pi[0] = 7;
      pi[1] = 11;
      pd[0] = 2.0;

      bool ok = (pi[0] + pi[1] == 18 && pd[0] == 2.0_d);

      st.template deallocate<int>(pi);
      st.template deallocate<double>(pd);
      return ok;
    };
    static_assert(test());
    expect(test());
  };
};

int main(int argc, char **argv) { return 0; }
