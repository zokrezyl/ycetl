#include <boost/ut.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/type_system.hpp>

using namespace boost::ut;
using namespace ycetl;
using namespace ycetl::memory;

// Minimal dummy backend to test allocations
template <typename T> struct dummy_backend {
  T buffer[10];

  constexpr T *allocate(std::size_t n) { return buffer; }

  constexpr void deallocate(T *) { /* trivial for test */ }
};

suite multitype_memory_downgrade_suite = [] {
  "memory_downgrade_and_allocate"_test = [] {
    auto test = [] {
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

    expect(test());
  };
};

int main() {}
