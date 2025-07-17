
#include <boost/ut.hpp>
#include <ycetl/dynamic_array.hpp>
#include <ycetl/impl/dynamic_memory.hpp>
#include <ycetl/impl/multitype_memory.hpp>
#include <ycetl/types.hpp>

using namespace boost::ut;
namespace mem = ycetl::memory;

suite dynamic_array_suite = [] {
  "dynamic_array_basic"_test = [] {
    constexpr auto test = [] {
      using memory_t =
          mem::multitype_memory<mem::dynamic_memory, ycetl::type_set<int>>;
      memory_t memory;

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
    static_assert(test());
    expect(test());
  };

  "dynamic_array_copy"_test = [] {
    constexpr auto test = [] {
      using memory_t =
          mem::multitype_memory<mem::dynamic_memory, ycetl::type_set<int>>;
      memory_t memory;

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
      using memory_t =
          mem::multitype_memory<mem::dynamic_memory, ycetl::type_set<int>>;
      memory_t memory;

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
      using memory_t =
          mem::multitype_memory<mem::dynamic_memory, ycetl::type_set<int>>;
      memory_t memory;

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
};

int main() { return 0; }
