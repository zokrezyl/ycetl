
#include <boost/ut.hpp>
#include <iostream> // Needed for iostream include in original compile command, though not directly used in these tests
#include <numeric>  // For std::iota
#include <ycetl/dynamic_array.hpp>
#include <ycetl/impl/dynamic_memory.hpp>
#include <ycetl/impl/multitype_memory.hpp>
#include <ycetl/types.hpp>

using namespace boost::ut;
namespace mem = ycetl::memory;

// Helper macro for memory setup.
// Note: `dynamic_memory` is a runtime allocator, so any test using it
// cannot be fully evaluated at compile-time by `static_assert`.
#define MAKE_MULTITYPE_MEMORY_FOR_INT                                          \
  using memory_t =                                                             \
      mem::multitype_memory<mem::dynamic_memory, ycetl::type_set<int>>;        \
  memory_t memory;

suite dynamic_array_suite = [] {
  "dynamic_array_basic"_test = [] {
    constexpr auto test = [] {       // Lambda itself can be constexpr
      MAKE_MULTITYPE_MEMORY_FOR_INT; // This makes the *invocation* of test()
                                     // non-constexpr
      ycetl::dynamic_array<int> arr(memory, 4);
      for (int i = 0; i < 4; ++i) {
        std::construct_at(arr.data() + i);
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
      ycetl::dynamic_array<int> original(memory, 3);
      for (int i = 0; i < 3; ++i) {
        std::construct_at(original.data() + i);
        original[i] = i + 10;
      }
      ycetl::dynamic_array<int> copy(memory, original);
      return copy.size() == original.size() && copy[2] == 12;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_move"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      ycetl::dynamic_array<int> arr(memory, 3);
      for (int i = 0; i < 3; ++i) {
        std::construct_at(arr.data() + i);
        arr[i] = i;
      }
      ycetl::dynamic_array<int> moved = std::move(arr);
      return moved.size() == 3 && moved[1] == 1;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_resize"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      ycetl::dynamic_array<int> arr(memory, 2);
      std::construct_at(arr.data(), 1);
      std::construct_at(arr.data() + 1, 2);

      arr.resize(memory, 4);
      std::construct_at(arr.data() + 2, 3);
      std::construct_at(arr.data() + 3, 4);

      bool expanded = arr.size() == 4 && arr[2] == 3 && arr[3] == 4;

      arr.resize(memory, 1);
      bool shrunk = arr.size() == 1 && arr[0] == 1;

      return expanded && shrunk;
    };
    static_assert(test());
    expect(test());
  };

  // --- Iterator Tests ---
  "dynamic_array_iterator_dereference"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      ycetl::dynamic_array<int> arr(memory, 1, 42);
      return *arr.begin() == 42;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_increment_pre"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      ycetl::dynamic_array<int> arr(memory, 2);
      std::construct_at(arr.data(), 10);
      std::construct_at(arr.data() + 1, 20);
      auto it = arr.begin();
      return *(++it) == 20;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_increment_post"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      ycetl::dynamic_array<int> arr(memory, 2);
      std::construct_at(arr.data(), 10);
      std::construct_at(arr.data() + 1, 20);
      auto it = arr.begin();
      bool val_before =
          (*(it++) == 10); // it_orig holds original, it is incremented
      bool val_after = (*it == 20);
      return val_before && val_after;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_iterator_decrement_pre"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      ycetl::dynamic_array<int> arr(memory, 2);
      std::construct_at(arr.data(), 10);
      std::construct_at(arr.data() + 1, 20);
      auto it = arr.end(); // Points past the last element
      return *(--it) == 20;
    };
    static_assert(test());
    expect(test());
  };
  "dynamic_array_iterator_decrement_post"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY_FOR_INT;
      ycetl::dynamic_array<int> arr(memory, 2);
      std::construct_at(arr.data(), 10);
      std::construct_at(arr.data() + 1, 20);

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
      ycetl::dynamic_array<int> arr(memory, 3);
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
      ycetl::dynamic_array<int> arr(memory, 3);
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
      ycetl::dynamic_array<int> arr(memory, 5);
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
      ycetl::dynamic_array<int> arr(memory, 3);
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
      ycetl::dynamic_array<int> arr(memory, 3);
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
      ycetl::dynamic_array<int> arr(memory, 1);
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
      ycetl::dynamic_array<int> arr(memory, 2);
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
      ycetl::dynamic_array<int> arr(memory, 2);
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
      ycetl::dynamic_array<int> arr(memory, 2);
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
      ycetl::dynamic_array<int> arr(memory, 2);
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
      ycetl::dynamic_array<int> arr(memory, 2);
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
      ycetl::dynamic_array<int> arr(memory, 3);
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
      ycetl::dynamic_array<int> arr(memory, 1, 55);
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
      ycetl::dynamic_array<int> arr(memory, 1, 99);
      ycetl::dynamic_array<int>::iterator it = arr.begin();
      ycetl::dynamic_array<int>::const_iterator cit = it;
      return *cit == 99;
    };
    static_assert(test());
    expect(test());
  };
};

// Non-member operator+ for iterator arithmetic (allows `int + iterator`)
#if 0
template <typename T, bool IsConst>
constexpr ycetl::dynamic_array<T>::basic_iterator<IsConst> operator+(
    typename ycetl::dynamic_array<T>::basic_iterator<IsConst>::difference_type
        n,
    ycetl::dynamic_array<T>::basic_iterator<IsConst> it) noexcept {
  return it + n;
}
#endif

int main() {}
