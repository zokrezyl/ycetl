#include <boost/ut.hpp>
#include <iostream>
#include <ycetl/impl/allocator.hpp>
#include <ycetl/impl/dynamic_allocator.hpp>
#include <ycetl/impl/multitype.hpp>
#include <ycetl/impl/multitype_allocator.hpp>
#include <ycetl/vector.hpp>

using namespace boost::ut;
using ycetl::vector;

// Simple test structure for the type set
struct Test {
  int value;
  constexpr Test(int v = 0) : value(v) {}
};

// Define the macro using parentheses for type to handle template arguments with
// commas
#define MAKE_MULTITYPE_ALLOCATOR                                               \
  using working_type_set =                                                     \
      ycetl::type_set<int, double, char, Test, std::pair<int, int>>;           \
  auto alloc =                                                                 \
      ycetl::memory::multitype_allocator<ycetl::memory::dynamic_allocator,     \
                                         working_type_set>();

suite vector_suite = [] {
  "empty_vector"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_ALLOCATOR;
      vector<int, decltype(alloc)> v(alloc);
      return v.size() == 0_u && v.capacity() == 0_u;
    };
    expect(test());
  };

  "vector_with_size"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_ALLOCATOR;
      vector<int, decltype(alloc)> v(
          8, alloc); // Creates 8 default-initialized elements
      return v.size() == 8_u && v.capacity() >= 8_u;
    };
    expect(test());
  };

  "vector_with_reserve"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_ALLOCATOR;
      vector<int, decltype(alloc)> v(alloc);
      v.reserve(8);
      return v.size() == 0_u && v.capacity() == 8_u;
    };
    expect(test());
  };

  "fill_capacity"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_ALLOCATOR;
      vector<int, decltype(alloc)> p(alloc);
      p.reserve(4); // Reserve space first
      for (int i = 0; i < 4; ++i)
        p.push_back(i);
      return p.size() == 4_u && p[3] == 3_i;
    };
    expect(test());
  };

  "fill_100"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_ALLOCATOR;
      vector<int, decltype(alloc)> p(alloc);
      p.reserve(100); // Reserve space first
      for (int i = 0; i < 100; ++i)
        p.push_back(i);
      return p.size() == 100_u && p[99] == 99_i;
    };
    expect(test());
  };

  "fill_with_value"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_ALLOCATOR;
      // This is the constructor that fills with a value: vector(size, value,
      // allocator)
      vector<int, decltype(alloc)> v(5, 42, alloc);
      return v.size() == 5_u && v[0] == 42_i && v[4] == 42_i;
    };
    expect(test());
  };

  "copy_construct"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_ALLOCATOR;
      vector<int, decltype(alloc)> a(alloc);
      a.push_back(10);
      a.push_back(20);
      a.push_back(30);
      vector<int, decltype(alloc)> b = a;
      return b.size() == 3_u && b[0] == 10_i && b[2] == 30_i;
    };
    expect(test());
  };

  "move_construct"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_ALLOCATOR;
      vector<int, decltype(alloc)> a(alloc);
      a.push_back(7);
      a.push_back(8);
      vector<int, decltype(alloc)> b = std::move(a);
      return b.size() == 2_u && b[0] == 7_i && b[1] == 8_i;
    };
    expect(test());
  };

  "clear_and_reuse"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_ALLOCATOR;
      vector<int, decltype(alloc)> v(alloc);
      v.push_back(1);
      v.push_back(2);
      v.clear();
      v.push_back(42);
      return v.size() == 1_u && v[0] == 42_i;
    };
    expect(test());
  };

  "initializer_list"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_ALLOCATOR;
      vector<int, decltype(alloc)> v({1, 2, 3}, alloc);
      return v.size() == 3_u && v[1] == 2_i;
    };
    expect(test());
  };
#if 0
  "forward_iterator_construct"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_ALLOCATOR(int);
      int arr[3] = {4, 5, 6};
      vector<int, decltype(alloc)> v(arr, arr + 3, alloc);
      return v.size() == 3_u && v[2] == 6_i;
    };
    expect(test());
  };
#endif
  "insert_value"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_ALLOCATOR;
      vector<int, decltype(alloc)> v({1, 2, 4}, alloc);
      v.insert(v.begin() + 2, 3);
      return v.size() == 4_u && v[2] == 3_i && v[3] == 4_i;
    };
    expect(test());
  };

  "emplace_back_pair"_test = [] {
    constexpr auto test = [] {
      // Use parentheses around template types with commas
      using pair_type = std::pair<int, int>;
      MAKE_MULTITYPE_ALLOCATOR;
      vector<pair_type, decltype(alloc)> v(alloc);
      v.emplace_back(9, 10);
      return v.size() == 1_u && v[0].first == 9_i && v[0].second == 10_i;
    };
    expect(test());
  };

  "custom_type"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_ALLOCATOR;
      vector<Test, decltype(alloc)> v(alloc);
      v.push_back(Test(42));
      return v.size() == 1_u && v[0].value == 42_i;
    };
    expect(test());
  };

#if 0
  "nested_vectors"_test = [] {
    constexpr auto test = [] {
      // Single storage for all types
      using storage_type = ycetl::memory::multitype_storage<
          ycetl::memory::dynamic_storage,
          ycetl::type_set<int, double, char, Test, std::pair<int, int>>>;
      auto storage = storage_type();

      // Allocators for both int and vector<int>
      auto int_alloc = ycetl::allocator::allocator<int, storage_type>(storage);

      // Create a few inner vectors
      ycetl::vector<int, decltype(int_alloc)> inner1(int_alloc);
      inner1.push_back(42);
      inner1.push_back(43);

      // Define the vector type we want to store
      using inner_vector_type = ycetl::vector<int, decltype(int_alloc)>;

      // Extend the storage to include the vector type
      using extended_storage_type = ycetl::memory::multitype_storage<
          ycetl::memory::dynamic_storage,
          ycetl::type_set<int, double, char, Test, std::pair<int, int>,
                          inner_vector_type>>;
      auto extended_storage = extended_storage_type();

      // Create allocator for vectors
      auto vector_alloc =
          ycetl::allocator::allocator<inner_vector_type, extended_storage_type>(
              extended_storage);

      // Create outer vector
      ycetl::vector<inner_vector_type, decltype(vector_alloc)> outer(
          vector_alloc);

      // Test operations
      outer.push_back(inner1);

      return outer.size() == 1_u && outer[0].size() == 2_u &&
             outer[0][0] == 42_i;
    };
    expect(test());
  };
#endif
};

int main(int argc, char **argv) { return 0; }
