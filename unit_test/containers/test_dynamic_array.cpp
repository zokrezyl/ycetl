#include <boost/ut.hpp>
#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/types.hpp>

using namespace boost::ut;
using namespace ycetl;

struct NonTrivial {
  int id;
  constexpr NonTrivial(int i = 0) : id(i) {}
  constexpr bool operator==(const NonTrivial &other) const {
    return id == other.id;
  }
};

suite dynamic_array_suite_1 = [] {
  "default_construction"_test = [] {
    constexpr auto test_lambda = [] {
      dynamic_array<int> arr;
      return arr.size() == 0_u && arr.capacity() == 0_u;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };
  "size_constructor"_test = [] {
    auto test_lambda = [] {
      default_memory<int> alloc{};
      dynamic_array<int> arr(alloc, 5);
      bool ok = arr.size() == 5_u && arr.capacity() == 5_u;
      for (auto v : arr)
        ok = ok && v == 0;
      // arr.clear_and_deallocate_buffer(alloc);
      return ok;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };

  "initializer_list_constructor"_test = [] {
    auto test_lambda = [] {
      default_memory<int> alloc{};
      dynamic_array<int> arr(alloc, {10, 20, 30});
      bool ok = arr.size() == 3_u && arr[0] == 10 && arr[2] == 30;
      // arr.clear_and_deallocate_buffer(alloc);
      return ok;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };

  "push_back_nontrivial"_test = [] {
    auto test_lambda = [] {
      default_memory<NonTrivial> alloc{};
      dynamic_array<NonTrivial> arr(alloc);
      arr.push_back(NonTrivial(1));
      arr.push_back(NonTrivial(2));
      bool ok = arr.size() == 2_u && arr[0].id == 1 && arr[1].id == 2;
      // arr.clear_and_deallocate_buffer(alloc);
      return ok;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };

  "insert_middle"_test = [] {
    auto test_lambda = [] {
      default_memory<int> alloc{};
      dynamic_array<int> arr(alloc, {1, 2, 4});
      arr.insert(arr.begin() + 2, 3);
      bool ok = arr.size() == 4_u && arr[2] == 3;
      // arr.clear_and_deallocate_buffer(alloc);
      return ok;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };

  "resize_grow"_test = [] {
    auto test_lambda = [] {
      default_memory<int> alloc{};
      dynamic_array<int> arr(alloc, {1, 2});
      arr.resize(5);
      bool ok = arr.size() == 5_u && arr[2] == 0 && arr[4] == 0;
      // arr.clear_and_deallocate_buffer(alloc);
      return ok;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };

  "clear_and_deallocate_buffer_0"_test = [] {
    auto test_lambda = [] {
      default_memory<int> alloc{};
      dynamic_array<int> arr(alloc, {1, 2, 3});
      // arr.clear_and_deallocate_buffer(alloc);
      arr.clear();
      return arr.size() == 0_u;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };
  "clear_and_deallocate_buffer_1"_test = [] {
    auto test_lambda = [] {
      default_memory<int> alloc{};
      dynamic_array<int> arr(alloc, {1, 2, 3});
      // arr.clear_and_deallocate_buffer(alloc);
      arr.clear();
      return arr.capacity() == 3_u;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };
  "clear_and_deallocate_buffer_2"_test = [] {
    auto test_lambda = [] {
      default_memory<int> alloc{};
      dynamic_array<int> arr(alloc, {1, 2, 3});
      // arr.clear_and_deallocate_buffer(alloc);
      arr.clear();
      // TODO clear should set the pointers to nullptr
      return arr.begin() == nullptr;
    };
    // static_assert(test_lambda());
    expect(test_lambda());
  };
};

suite dynamic_array_suite_2 = [] {
  "dynamic_array_basic"_test = [] {
    constexpr auto test = [] {
      using memory_t = default_memory<int>;
      memory_t memory;

      dynamic_array<int> arr(memory, 4);
      for (int i = 0; i < 4; ++i) {
        arr[i] = i + 1;
      }

      int sum = 0;
      for (int i = 0; i < 4; ++i)
        sum += arr[i];

      return sum == 10;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_copy"_test = [] {
    constexpr auto test = [] {
      using memory_t = default_memory<int>;
      memory_t memory;

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
      using memory_t = default_memory<int>;
      memory_t memory;

      dynamic_array<int> arr(memory, 3);
      for (int i = 0; i < 3; ++i) {
        // std::construct_at(arr.data() + i);
        arr[i] = i;
      }

      dynamic_array<int> moved = std::move(arr);
      return moved.size() == 3 && moved[1] == 1;
    };
    static_assert(test());
    expect(test());
  };

  "dynamic_array_resize"_test = [] {
    constexpr auto test = [] {
      using memory_t = default_memory<int>;
      memory_t memory;

      dynamic_array<int> arr(memory, 2);
      arr[0] = 1;
      arr[1] = 2;

      arr.resize(4);
      arr[2] = 3;
      arr[3] = 4;

      bool expanded = arr.size() == 4 && arr[2] == 3 && arr[3] == 4;

      arr.resize(1);
      bool shrunk = arr.size() == 1 && arr[0] == 1;

      return expanded && shrunk;
    };
    static_assert(test());
    expect(test());
  };
};

int main() { return 0; }
