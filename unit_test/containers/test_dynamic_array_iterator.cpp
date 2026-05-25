// SPDX-License-Identifier: MIT

#include <iostream> // Needed for iostream include in original compile command, though not directly used in these tests
#include <numeric>  // For std::iota
#include <ycetl/dynamic_array.hpp>
#include <ycetl/impl/type_printer.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/types.hpp>

#include <boost/ut.hpp>
using namespace ycetl;

using namespace boost::ut;
// Helper macro for memory setup.
// Note: `dynamic_memory` is a runtime allocator, so any test using it
// cannot be fully evaluated at compile-time by `static_assert`.
#define MAKE_MULTITYPE_MEMORY_FOR_INT                                          \
  using memory_t = default_memory<int>;                                        \
  memory_t memory;

suite dynamic_array_suite = [] {
  "dynamic_array_basic"_test = [] {
    constexpr auto test = [] {       // Lambda itself can be constexpr
      MAKE_MULTITYPE_MEMORY_FOR_INT; // This makes the *invocation* of test()
                                     // non-constexpr
      dynamic_array<int> arr(memory, 4);
      for (int i = 0; i < 4; ++i) {
        arr[i] = i + 1;
      }
      int sum = 0;
      for (int i = 0; i < 4; ++i)
        sum += arr[i];
      return sum == 10;
    };
    static_assert(test()); // Cannot compile due to dynamic_memory
    expect(test());        // Runs at runtime
  };

  "dynamic_array_copy"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> original(memory, 3);
      for (int i = 0; i < 3; ++i) {
        original[i] = i + 10;
      }
      dynamic_array<int> copy(memory, original);
      return copy.size() == original.size() && copy[2] == 12;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_move"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 3);
      for (int i = 0; i < 3; ++i) {
        arr[i] = i;
      }
      dynamic_array<int> moved = std::move(arr);
      return moved.size() == 3 && moved[1] == 1;
    };
    static_assert(test());
    expect(test());
  };

  // --- Iterator Tests ---
  "dynamic_array_iterator_dereference"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 1, 42);
      return *arr.begin() == 42;
    };
    // static_assert(test());
    expect(test());
    std::cout << "result of dynamic_array_iterator_dereference: " << test()
              << std::endl;
  };

  "dynamic_array_iterator_increment_pre"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 2);
      arr[0] = 10;
      arr[1] = 20;
      auto it = arr.begin();
      return *(++it) == 20;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_increment_post"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 2);
      arr[0] = 10;
      arr[1] = 20;
      auto it = arr.begin();
      bool val_before = (*(it++)
                         == 10); // it_orig holds original, it is incremented
      bool val_after = (*it == 20);
      return val_before && val_after;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_decrement_pre"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 2);
      arr[0] = 10;
      arr[1] = 20;
      auto it = arr.end(); // Points past the last element
      return *(--it) == 20;
    };
    static_assert(test());
    expect(test());
  };
  "dynamic_array_iterator_decrement_post"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 2);
      arr[0] = 10;
      arr[1] = 20;

      auto it = arr.end();
      --it;
      bool val_before = (*(it--) == 20);
      bool val_after = (*it == 10);

      return val_before && val_after;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_plus_offset"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 3);
      std::iota(arr.data(), arr.data() + arr.size(), 1); // {1, 2, 3}
      auto it = arr.begin();
      return *(it + 2) == 3;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_minus_offset"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 3);
      std::iota(arr.data(), arr.data() + arr.size(), 1); // {1, 2, 3}
      auto it_end = arr.end();
      return *(it_end - 1) == 3;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_difference"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 5);
      auto it1 = arr.begin();
      auto it2 = arr.begin() + 3;
      return (it2 - it1) == 3;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_compound_plus_eq"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 3);
      std::iota(arr.data(), arr.data() + arr.size(), 10); // {10, 11, 12}
      auto it = arr.begin();
      it += 1;
      return *it == 11;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_compound_minus_eq"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 3);
      std::iota(arr.data(), arr.data() + arr.size(), 10); // {10, 11, 12}
      auto it = arr.begin() + 2;
      it -= 1;
      return *it == 11;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_comparison_equal"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 1);
      auto it1 = arr.begin();
      auto it2 = arr.begin();
      return it1 == it2;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_comparison_not_equal"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 2);
      auto it1 = arr.begin();
      auto it2 = arr.begin() + 1;
      return it1 != it2;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_comparison_less_than"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 2);
      auto it1 = arr.begin();
      auto it2 = arr.begin() + 1;
      return it1 < it2;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_comparison_greater_than"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 2);
      auto it1 = arr.begin();
      auto it2 = arr.begin() + 1;
      return it2 > it1;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_comparison_less_equal"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 2);
      auto it1 = arr.begin();
      auto it2 = arr.begin() + 1;
      return it1 <= it1 && it1 <= it2;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_comparison_greater_equal"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 2);
      auto it1 = arr.begin();
      auto it2 = arr.begin() + 1;
      return it2 >= it2 && it2 >= it1;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_subscript"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 3);
      std::iota(arr.data(), arr.data() + arr.size(), 100); // {100, 101, 102}
      auto it = arr.begin();
      return it[1] == 101;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_const_iterator_dereference"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 1, 55);
      const auto &const_arr = arr;
      auto cit = const_arr.begin();
      return *cit == 55;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_conversion_to_const"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      dynamic_array<int> arr(memory, 1, 99);
      dynamic_array<int>::iterator it = arr.begin();
      dynamic_array<int>::const_iterator cit = it;
      return *cit == 99;
    };
    static_assert(test());
    expect(test());
  };
};

// Non-member operator+ for iterator arithmetic (allows `int + iterator`)
#if 0
template <typename T, bool IsConst>
constexpr dynamic_array<T>::basic_iterator<IsConst> operator+(
    typename dynamic_array<T>::basic_iterator<IsConst>::difference_type
        n,
    dynamic_array<T>::basic_iterator<IsConst> it) noexcept {
  return it + n;
}
#endif

int main() {}
