#include <boost/ut.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/trivial_shared_ptr.hpp>
#include <ycetl/type_system.hpp>

using namespace boost::ut;
using namespace ycetl;
using namespace ycetl::memory;

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

  "constexpr_trivial_shared_ptr_reference_count"_test = [] {
    constexpr auto test = [] {
      using larger_set = type_set<int, double>;
      using smaller_set = type_set<int>;

      multitype_memory<dummy_backend, larger_set> larger_memory;

      // explicitly downgrade memory
      multitype_memory<dummy_backend, smaller_set> smaller_memory(
          larger_memory);

      auto larger_handler =
          larger_memory
              .template get_handler<trivial_shared_ptr<dummy_backend<int>>>();
      auto smaller_handler =
          smaller_memory
              .template get_handler<trivial_shared_ptr<dummy_backend<int>>>();

      // Check explicitly if handlers share the same underlying pointer
      bool same_instance = larger_handler.get() == smaller_handler.get();

      // Allocate from smaller_memory
      int *ptr = smaller_memory.allocate<int>(3);
      ptr[0] = 1;
      ptr[2] = 3;

      // Check allocations
      bool allocation_correct = ptr[0] == 1 && ptr[2] == 3;

      return same_instance && allocation_correct;
    };

    static_assert(test());
    expect(test());
  };
};

int main() {}
