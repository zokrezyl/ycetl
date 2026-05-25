// SPDX-License-Identifier: MIT

#include <boost/ut.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/trivial_shared_ptr.hpp>
#include <ycetl/type_system.hpp>

using namespace boost::ut;
using namespace ycetl;

// Minimal dummy backend to test allocations
template <typename T> struct dummy_backend {
  T buffer[100];

  constexpr T *allocate(std::size_t n) { return buffer; }

  constexpr void deallocate(T *) { /* trivial for test */ }
};

suite multitype_memory_downgrade_suite = [] {
  "constexpr_memory_downgrade_and_allocate"_test = [] {
    constexpr auto test = [] {
      using original_set = type_set<int, double>;
      using smaller_set = type_set<int>;

      multitype_memory<dummy_backend, original_set> original_memory;

      // explicitly downgrade
      multitype_memory<dummy_backend, smaller_set> downgraded_memory(
          original_memory);

      // Allocate using downgraded memory
      int *int_ptr = downgraded_memory.allocate<int>(5);
      int_ptr[0] = 42;
      int_ptr[4] = 100;

      // Check correctness
      return int_ptr[0] == 42 && int_ptr[4] == 100;
    };

    static_assert(test());
    expect(test());
  };

  "constexpr_memory_downgrade_and_allocate"_test = [] {
    constexpr auto test = [] {
      multitype_memory<dummy_backend, int, double> original_memory;

      // explicitly downgrade
      multitype_memory<dummy_backend, int> downgraded_memory(original_memory);

      // Allocate using downgraded memory
      int *int_ptr = downgraded_memory.allocate<int>(5);
      int_ptr[0] = 42;
      int_ptr[4] = 100;

      // Check correctness
      return int_ptr[0] == 42 && int_ptr[4] == 100;
    };

    static_assert(test());
    expect(test());
  };
};

int main() {}
