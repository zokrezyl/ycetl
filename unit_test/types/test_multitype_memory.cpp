#include <boost/ut.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/trivial_shared_ptr.hpp>
#include <ycetl/type_system.hpp>

using namespace boost::ut;
using namespace ycetl;

suite multitype_memory_suite = [] {
  "multitype_dynamic_memory_allocate"_test = [] {
    constexpr auto test = [] {
      using types = type_set<int, double>;
      multitype_memory<typed_dynamic_memory, types> mem;

      auto int_ptr = allocate<int>(mem, 10);
      auto double_ptr = allocate<double>(mem, 5);

      bool result = (int_ptr != nullptr) && (double_ptr != nullptr);

      deallocate<int>(mem, int_ptr, 10);
      deallocate<double>(mem, double_ptr, 5);

      return true;
    };
    static_assert(test());
    expect(test());
  };
  "multitype_static_memory_allocate"_test = [] {
    constexpr auto test = [] {
      multitype_memory<typed_static_memory, int, double, float> mem;

      auto int_ptr = mem.allocate<int>(10);
      auto double_ptr = mem.allocate<double>(5);

      // bool result = (int_ptr != nullptr) && (double_ptr != nullptr);

      deallocate<int>(mem, int_ptr, 10);
      deallocate<double>(mem, double_ptr, 5);

      return true;
    };
    static_assert(test());
    expect(test());
  };
  "multitype_static_memory_to_runtime"_test = [] {
    constexpr auto test = [] {
      static_memory<int, double, float> mem;

      auto int_ptr = allocate_and_construct_n<int>(mem, 10, 0);
      auto double_ptr = allocate_and_construct_n<double>(mem, 5, 0);

      // bool result = (int_ptr != nullptr) && (double_ptr != nullptr);

      deallocate<int>(mem, int_ptr, 10);
      deallocate<double>(mem, double_ptr, 5);

      return mem;
    };
    constexpr auto mem = test();
    constexpr auto int_mem = get<int>(mem);
    constexpr auto double_mem = get<double>(mem);
    static_assert(int_mem.allocated_size() == 10);
    static_assert(double_mem.allocated_size() == 5);
    static_assert(int_mem[0] == 0);
    static_assert(double_mem[0] == 0);
    multitype_memory<typed_static_memory, int, double, float> copy = mem;
    expect(get<double>(copy).allocated_size() == 5);
    expect(get<int>(copy).allocated_size() == 10);

    for (int i = 0; i < 10; ++i) {
      expect(int_mem[i] == 0);
    }
  };
};

int main(int, char **) { return 0; }
