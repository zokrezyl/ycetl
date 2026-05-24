#include <boost/ut.hpp>
#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/types.hpp>

using namespace boost::ut;
using namespace ycetl;

template <class T>
using nested_array_allocator = default_memory<type_set<T, dynamic_array<T>>>;

template <class T> using nested_array = dynamic_array<dynamic_array<T>>;

struct NonTrivial {
  int id;
  constexpr NonTrivial(int i = 0) : id(i) {}
  constexpr bool operator==(const NonTrivial &other) const {
    return id == other.id;
  }
};

suite dynamic_array_suite = [] {
  "default_construction"_test = [] {
    constexpr auto test_lambda = [] {
      nested_array<int> arr;
      return arr.size() == 0_u && arr.capacity() == 0_u;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };

  "size_constructor"_test = [] {
    auto test_lambda = [] {
      nested_array_allocator<int> alloc{};
      nested_array<int> arr(alloc, 5);
      bool ok = arr.size() == 5_u && arr.capacity() == 5_u;
#if 0
      for (auto v : arr)
        ok = ok && v.size() == 0;
#endif
      // arr.clear_and_deallocate_buffer(alloc);
      return ok;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };
#if 0
  "initializer_list_constructor"_test = [] {
    auto test_lambda = [] {
      nested_array_allocator<int> alloc{};
      nested_array<int> arr(alloc, {10, 20, 30});
      bool ok = arr.size() == 3_u && arr[0] == 10 && arr[2] == 30;
      arr.clear_and_deallocate_buffer(alloc);
      return ok;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };

  "push_back_nontrivial"_test = [] {
    auto test_lambda = [] {
      nested_array_allocator<NonTrivial> alloc{};
      nested_array<NonTrivial> arr(alloc);
      arr.push_back(alloc, NonTrivial(1));
      arr.push_back(alloc, NonTrivial(2));
      bool ok = arr.size() == 2_u && arr[0].id == 1 && arr[1].id == 2;
      arr.clear_and_deallocate_buffer(alloc);
      return ok;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };

  "insert_middle"_test = [] {
    auto test_lambda = [] {
      nested_array_allocator<int> alloc{};
      nested_array<int> arr(alloc, {1, 2, 4});
      arr.insert(alloc, arr.begin() + 2, 3);
      bool ok = arr.size() == 4_u && arr[2] == 3;
      arr.clear_and_deallocate_buffer(alloc);
      return ok;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };

  "resize_grow"_test = [] {
    auto test_lambda = [] {
      nested_array_allocator<int> alloc{};
      nested_array<int> arr(alloc, {1, 2});
      arr.resize(alloc, 5);
      bool ok = arr.size() == 5_u && arr[2] == 0 && arr[4] == 0;
      arr.clear_and_deallocate_buffer(alloc);
      return ok;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };

  "clear_and_deallocate_buffer"_test = [] {
    auto test_lambda = [] {
      nested_array_allocator<int> alloc{};
      nested_array<int> arr(alloc, {1, 2, 3});
      arr.clear_and_deallocate_buffer(alloc);
      return arr.size() == 0_u && arr.capacity() == 0_u &&
             arr.begin() == nullptr;
    };
    static_assert(test_lambda());
    expect(test_lambda());
  };
#endif
};

int main() {}
