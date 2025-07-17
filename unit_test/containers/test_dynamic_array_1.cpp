#include <boost/ut.hpp>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp> // Assuming this contains your corrected memory.hpp content
#include <ycetl/types.hpp>

#include <new> // For std::construct_at, std::destroy_at
#include <numeric>
#include <utility> // For std::exchange

using namespace boost::ut;
namespace mem = ycetl::memory;
using ycetl::type_set;

// Use multitype_dynamic_memory as the test allocator type, as instructed.
template <class T>
using test_allocator = ycetl::multitype_dynamic_memory<type_set<T>>;

// A non-trivial type to test construction/destruction
struct NonTrivial {
  int id;
  constexpr NonTrivial(int i = 0) : id(i) {}
  ~NonTrivial() = default; // Non-constexpr destructor allowed at runtime
  NonTrivial(const NonTrivial &other) = default;
  NonTrivial &operator=(const NonTrivial &other) = default;
  NonTrivial(NonTrivial &&other) noexcept : id(std::exchange(other.id, -1)) {}
  NonTrivial &operator=(NonTrivial &&other) noexcept {
    id = std::exchange(other.id, -1);
    return *this;
  }
  constexpr bool operator==(const NonTrivial &other) const {
    return id == other.id;
  }
};

// Main test suite for dynamic_array
suite dynamic_array_suite = [] {
  // These allocators will now be multitype_dynamic_memory instances
  test_allocator<int> int_allocator{};
  test_allocator<double> double_allocator{};
  test_allocator<NonTrivial> nontrivial_allocator{};

  // --- Tests for dynamic_array operations using multitype_memory ---

  "default_construction"_test = [&] {
    constexpr auto test_lambda = [&] { // Inner lambda for the test logic
      ycetl::dynamic_array<int> arr{};
      return arr.size() == 0_u && arr.capacity() == 0_u;
    };
    // Use static_assert for truly constexpr tests, expect for runtime tests.
    // This test, if arr() is constexpr, could be static_assert.
    // If it requires runtime behavior due to allocator, use expect.
    expect(test_lambda()); // Use expect for runtime checks
  };

  "size_constructor"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator, 5);
      bool ok = (arr.size() == 5_u && arr.capacity() == 5_u);
      ok = ok && (arr.data() != nullptr);
      for (std::size_t i = 0; i < arr.size(); ++i) {
        ok = ok && (arr[i] == 0_i); // Check default-initialized values
      }
      return ok;
    };
    expect(test_lambda()); // Use expect due to dynamic allocation
  };

  "size_value_constructor"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator, 3, 42);
      bool ok = (arr.size() == 3_u && arr.capacity() == 3_u);
      ok = ok && (arr.data() != nullptr);
      for (std::size_t i = 0; i < arr.size(); ++i) {
        ok = ok && (arr[i] == 42_i);
      }
      arr.clear_and_deallocate_buffer(int_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "range_constructor"_test = [&] {
    constexpr auto test_lambda = [&] {
      int initial_data[] = {1, 2, 3, 4};
      ycetl::dynamic_array<int> arr(int_allocator, std::begin(initial_data), 4);
      bool ok = (arr.size() == 4_u && arr.capacity() == 4_u);
      ok = ok && (arr[0] == 1_i);
      ok = ok && (arr[3] == 4_i);
      arr.clear_and_deallocate_buffer(int_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "initializer_list_constructor"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator, {10, 20, 30});
      bool ok = (arr.size() == 3_u && arr.capacity() == 3_u);
      ok = ok && (arr[0] == 10_i);
      ok = ok && (arr[2] == 30_i);
      arr.clear_and_deallocate_buffer(int_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "copy_move_constructor"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr1(int_allocator, {100, 200, 300});
      ycetl::dynamic_array<int> arr2(std::move(arr1)); // Move construction

      bool ok = (arr2.size() == 3_u && arr2.capacity() == 3_u);
      ok = ok && (arr2[0] == 100_i);
      ok = ok && (arr2[2] == 300_i);

      // Original array should be in a valid, but unspecified state (typically
      // empty/null)
      ok = ok && (arr1.begin() == nullptr);
      ok = ok && (arr1.size() == 0_u);
      ok = ok && (arr1.capacity() == 0_u);

      arr2.clear_and_deallocate_buffer(int_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "push_back_int"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator);
      arr.push_back(int_allocator, 10);
      bool ok = (arr.size() == 1_u && arr[0] == 10_i);
      arr.push_back(int_allocator, 20);
      ok = ok && (arr.size() == 2_u && arr[1] == 20_i);
      arr.push_back(int_allocator, 30);
      ok = ok && (arr.size() == 3_u && arr[2] == 30_i);
      ok = ok && (arr.capacity() >= 3_u);
      arr.clear_and_deallocate_buffer(int_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "push_back_nontrivial"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<NonTrivial> arr(nontrivial_allocator);
      arr.push_back(nontrivial_allocator, NonTrivial(1));
      bool ok = (arr.size() == 1_u && arr[0].id == 1_i);
      arr.push_back(nontrivial_allocator, NonTrivial(2));
      ok = ok && (arr.size() == 2_u && arr[1].id == 2_i);
      arr.clear_and_deallocate_buffer(nontrivial_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "emplace_back"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<NonTrivial> arr(nontrivial_allocator);
      NonTrivial *p = arr.emplace_back(nontrivial_allocator, 50);
      bool ok = (arr.size() == 1_u && arr[0].id == 50_i);
      ok = ok && (p == arr.data()); // Pointer returned should be to the element
      arr.clear_and_deallocate_buffer(nontrivial_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "pop_back"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator, {1, 2, 3});
      arr.pop_back();
      bool ok = (arr.size() == 2_u && arr[1] == 2_i);
      arr.pop_back();
      ok = ok && (arr.size() == 1_u && arr[0] == 1_i);
      arr.pop_back();
      ok = ok && (arr.size() == 0_u);
      arr.clear_and_deallocate_buffer(int_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "reserve_grow"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator);
      bool ok = (arr.capacity() == 0_u);
      arr.reserve(int_allocator, 10);
      ok = ok && (arr.capacity() == 10_u);
      ok = ok && (arr.size() == 0_u);
      arr.clear_and_deallocate_buffer(int_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "reserve_no_grow"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator, 5); // Capacity 5
      bool ok = (arr.capacity() == 5_u);
      arr.reserve(int_allocator, 3);      // Request smaller capacity
      ok = ok && (arr.capacity() == 5_u); // Should not shrink
      arr.clear_and_deallocate_buffer(int_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "resize_shrink"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator, {1, 2, 3, 4, 5});
      arr.resize(int_allocator, 3);
      bool ok = (arr.size() == 3_u);
      ok = ok && (arr[2] == 3_i);
      arr.clear_and_deallocate_buffer(int_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "resize_grow"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator, {1, 2});
      arr.resize(int_allocator, 5);
      bool ok = (arr.size() == 5_u);
      ok = ok && (arr.capacity() >= 5_u);
      ok = ok && (arr[0] == 1_i);
      ok = ok && (arr[1] == 2_i);
      ok = ok && (arr[2] == 0_i); // Default constructed
      ok = ok && (arr[4] == 0_i); // Default constructed
      arr.clear_and_deallocate_buffer(int_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "clear"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator, {1, 2, 3});
      arr.clear();
      bool ok = (arr.size() == 0_u);
      ok = ok &&
           (arr.capacity() >= 3_u); // Capacity usually retained after clear
      ok = ok && (arr.data() != nullptr); // Buffer is still there
      arr.clear_and_deallocate_buffer(
          int_allocator); // Deallocate for test cleanup
      return ok;
    };
    expect(test_lambda());
  };

  "clear_and_deallocate_buffer"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator, {1, 2, 3});
      arr.clear_and_deallocate_buffer(int_allocator);
      bool ok = (arr.size() == 0_u);
      ok = ok && (arr.capacity() == 0_u);
      ok = ok && (arr.data() == nullptr);
      return ok;
    };
    expect(test_lambda());
  };

  "insert_middle"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator, {1, 2, 4});
      arr.insert(int_allocator, arr.begin() + 2, 3); // Insert 3 at index 2
      bool ok = (arr.size() == 4_u);
      ok = ok && (arr[0] == 1_i);
      ok = ok && (arr[1] == 2_i);
      ok = ok && (arr[2] == 3_i);
      ok = ok && (arr[3] == 4_i);
      arr.clear_and_deallocate_buffer(int_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "insert_begin"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator, {2, 3});
      arr.insert(int_allocator, arr.begin(), 1); // Insert 1 at beginning
      bool ok = (arr.size() == 3_u);
      ok = ok && (arr[0] == 1_i);
      ok = ok && (arr[1] == 2_i);
      ok = ok && (arr[2] == 3_i);
      arr.clear_and_deallocate_buffer(int_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  "insert_end"_test = [&] {
    constexpr auto test_lambda = [&] {
      ycetl::dynamic_array<int> arr(int_allocator, {1, 2});
      arr.insert(int_allocator, arr.end(), 3); // Insert 3 at end
      bool ok = (arr.size() == 3_u);
      ok = ok && (arr[0] == 1_i);
      ok = ok && (arr[1] == 2_i);
      ok = ok && (arr[2] == 3_i);
      arr.clear_and_deallocate_buffer(int_allocator);
      return ok;
    };
    expect(test_lambda());
  };

  // Your specific test example, adjusted for multitype_memory and proper
  // Boost.UT syntax
  "FUCKING_TEST_EXAMPLE_FOR_GOOGLE_IDIIOT"_test = [&] {
    constexpr auto test_lambda = [&] {
      using memory_t =
          mem::multitype_memory<mem::dynamic_memory, type_set<int>>;
      memory_t memory; // This allocates an internal dynamic_memory instance

      ycetl::dynamic_array<int> arr(memory,
                                    4); // Uses the multitype_memory instance
      for (int i = 0; i < 4; ++i) {
        arr[i] = i + 1;
      }

      int sum = 0;
      for (int i = 0; i < 4; ++i)
        sum += arr[i];

      // Cleanup: Must deallocate the buffer used by dynamic_array
      arr.clear_and_deallocate_buffer(
          memory); // Pass the same memory instance for deallocation

      return sum == 10;
    };
    // This test involves dynamic allocation (from dynamic_memory), so
    // static_assert will likely fail. static_assert(test_lambda()); //
    // Uncomment if your dynamic_memory is fully constexpr
    expect(test_lambda()); // Use expect for runtime execution
  };

  // Another example of direct multitype_memory usage, similar to your previous
  // "multitype_alloc_int_double"_test
  "multitype_direct_usage"_test = [] {
    constexpr auto test_lambda = [] {
      using types = type_set<int, double>;
      mem::multitype_memory<mem::dynamic_memory, types> st;

      int *pi = st.template allocate<int>(2);
      double *pd = st.template allocate<double>(1);

      pi[0] = 7;
      pi[1] = 11;
      pd[0] = 2.0;

      bool ok = (pi[0] + pi[1] == 18 && pd[0] == 2.0_d);

      st.template deallocate<int>(pi, 2);    // Deallocate with size
      st.template deallocate<double>(pd, 1); // Deallocate with size
      return ok;
    };
    expect(test_lambda()); // Use expect due to dynamic allocation
  };
};
