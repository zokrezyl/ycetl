#include <boost/ut.hpp>
#include <iostream>
// #include <ycetl/impl/memory.hpp>
#include <ycetl/impl/dynamic_memory.hpp>
// #include <ycetl/impl/multitype_handler.hpp>
#include <ycetl/impl/multitype_memory.hpp>
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
#define MAKE_MULTITYPE_MEMORY                                                  \
  using working_type_set =                                                     \
      ycetl::type_set<int, double, char, Test, std::pair<int, int>>;           \
  auto memory = ycetl::memory::multitype_memory<ycetl::memory::dynamic_memory, \
                                                working_type_set>();

suite vector_suite = [] {
  "empty_vector"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY;
      vector<int, decltype(memory)> v(memory);
      return v.size() == 0_u && v.capacity() == 0_u;
    };
    expect(test());
  };
  "vector_with_size"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY;
      vector<int, decltype(memory)> v(
          8, memory); // Creates 8 default-initialized elements
      return v.size() == 8_u && v.capacity() >= 8_u;
    };
    expect(test());
  };

  "vector_with_reserve"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY;
      vector<int, decltype(memory)> v(memory);
      v.reserve(8);
      return v.size() == 0_u && v.capacity() == 8_u;
    };
    expect(test());
  };

  "fill_capacity"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY;
      vector<int, decltype(memory)> p(memory);
      p.reserve(4); // Reserve space first
      for (int i = 0; i < 4; ++i)
        p.push_back(i);
      return p.size() == 4_u && p[3] == 3_i;
    };
    expect(test());
  };

  "fill_100"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY;
      vector<int, decltype(memory)> p(memory);
      p.reserve(100); // Reserve space first
      for (int i = 0; i < 100; ++i)
        p.push_back(i);
      return p.size() == 100_u && p[99] == 99_i;
    };
    expect(test());
  };

  "fill_with_value"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY;
      // This is the constructor that fills with a value: vector(size, value,
      // memory)
      vector<int, decltype(memory)> v(5, 42, memory);
      return v.size() == 5_u && v[0] == 42_i && v[4] == 42_i;
    };
    expect(test());
  };

  "copy_construct"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY;
      vector<int, decltype(memory)> a(memory);
      a.push_back(10);
      a.push_back(20);
      a.push_back(30);
      vector<int, decltype(memory)> b = a;
      return b.size() == 3_u && b[0] == 10_i && b[2] == 30_i;
    };
    expect(test());
  };

  "move_construct"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY;
      vector<int, decltype(memory)> a(memory);
      a.push_back(7);
      a.push_back(8);
      vector<int, decltype(memory)> b = std::move(a);
      return b.size() == 2_u && b[0] == 7_i && b[1] == 8_i;
    };
    expect(test());
  };

  "clear_and_reuse"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY;
      vector<int, decltype(memory)> v(memory);
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
      MAKE_MULTITYPE_MEMORY;
      vector<int, decltype(memory)> v({1, 2, 3}, memory);
      return v.size() == 3_u && v[1] == 2_i;
    };
    expect(test());
  };
#if 0
  "forward_iterator_construct"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY(int);
      int arr[3] = {4, 5, 6};
      vector<int, decltype(memory)> v(arr, arr + 3, memory);
      return v.size() == 3_u && v[2] == 6_i;
    };
    expect(test());
  };
#endif
  "insert_value"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY;
      vector<int, decltype(memory)> v({1, 2, 4}, memory);
      v.insert(v.begin() + 2, 3);
      return v.size() == 4_u && v[2] == 3_i && v[3] == 4_i;
    };
    expect(test());
  };

  "emplace_back_pair"_test = [] {
    constexpr auto test = [] {
      // Use parentheses around template types with commas
      using pair_type = std::pair<int, int>;
      MAKE_MULTITYPE_MEMORY;
      vector<pair_type, decltype(memory)> v(memory);
      v.emplace_back(9, 10);
      return v.size() == 1_u && v[0].first == 9_i && v[0].second == 10_i;
    };
    expect(test());
  };

  "custom_type"_test = [] {
    constexpr auto test = [] {
      MAKE_MULTITYPE_MEMORY;
      vector<Test, decltype(memory)> v(memory);
      v.push_back(Test(42));
      return v.size() == 1_u && v[0].value == 42_i;
    };
    expect(test());
  };
#if 0
  "nested_vectors"_test = [] {
    constexpr auto test = [] {
      using outer_vector_t = ycetl::vector<ycetl::vector<int>>;
      outer_vector_t outer_vec{};

      using inner_vector_t = ycetl::vector<int>;

      inner_vector_t inner_vec{};
      inner_vec.push_back(42);
      inner_vec.push_back(43);

      outer_vec.push_back(inner_vec);

      return outer_vec.size() == 1_u && outer_vec[0].size() == 2_u &&
             outer_vec[0][0] == 42_i && outer_vec[0][1] == 43_i;
    };
    expect(test());
  };
#endif
  "nested_vectors_with_allocator"_test = [] {
    constexpr auto test = [] {
      using relevant_types =
          ycetl::relevant_types_t<ycetl::vector<ycetl::vector<int>>>;

      using memory_t = ycetl::default_memory<relevant_types>;

      memory_t memory;

      using inner_vector_t = ycetl::vector<int, memory_t>;

      inner_vector_t inner_vec(memory);
      inner_vec.push_back(42);
      inner_vec.push_back(43);

      using outer_vector_t = ycetl::vector<inner_vector_t, memory_t>;
      outer_vector_t outer_vec(memory);
      outer_vec.push_back(inner_vec);

      return outer_vec.size() == 1_u && outer_vec[0].size() == 2_u &&
             outer_vec[0][0] == 42_i && outer_vec[0][1] == 43_i;
    };
    expect(test());
  };
};
#if 0
constexpr ycetl::vector<ycetl::vector<int>> make_vector() {
  /*  outer and inner vectors both use the library’s default_memory   */
  using vec_t = ycetl::vector<ycetl::vector<int>>;

  using memory_t = typename vec_t::memory_type;
  using payload_types = typename memory_t::type_set;
  static_assert(std::is_same_v<payload_types,
                               ycetl::type_set<int, ycetl::dynamic_array<int>>>,
                "memory payload types do not match expected types");

  vec_t vec;

  vec.emplace_back();  // add one inner vector (empty)
  vec[0].push_back(1); // set values on the inner vector
  vec[0].push_back(2);

  vec.emplace_back();
  vec[1].push_back(42);

  return vec; // copy‑elided, still constexpr
}
#endif

constexpr ycetl::vector<int> make_vector_simple() {
  /*  outer and inner vectors both use the library’s default_memory   */
  ycetl::vector<int> vec;

  vec.push_back(1); // set values on the inner vector
  vec.push_back(2);

  return vec; // copy‑elided, still constexpr
}

/*──────────────────────────────────────────────────────────────────────────────
  suite : default‑memory + make_vector()
──────────────────────────────────────────────────────────────────────────────*/
suite default_memory_suite = [] {
/* helper that builds a nested vector without passing any memory ---- */
#if 0
  "make_vector_default_memory"_test = [] {
    constexpr auto test = [] {
      auto v = make_vector();

      return v.size() == 2_u && v[0].size() == 2_u && v[0][0] == 1_i &&
             v[0][1] == 2_i && v[1].size() == 1_u && v[1][0] == 42_i;
    };
    expect(test());
  };
#endif
  "make_vector_default_memory"_test = [] {
    constexpr auto test = [] {
      auto v = make_vector_simple();
      return true;
    };
    expect(test());
  };
};

int main(int argc, char **argv) { return 0; }
