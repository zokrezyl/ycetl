#include <boost/ut.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/trivial_shared_ptr.hpp>
#include <ycetl/type_system.hpp>

using namespace boost::ut;
using namespace ycetl;
using namespace ycetl::memory;

suite multitype_memory_suite = [] {
  "multitype_memory_allocate"_test = [] {
    constexpr auto test = [] {
      using types = type_set<int, double>;
      multitype_memory<dynamic_memory, types> mem;

      int *int_ptr = mem.allocate<int>(10);
      double *double_ptr = mem.allocate<double>(5);

      bool result = (int_ptr != nullptr) && (double_ptr != nullptr);

      mem.deallocate(int_ptr);
      mem.deallocate(double_ptr);

      return result;
    };

    expect(test());
  };

  "multitype_memory_shared_handlers"_test = [] {
    constexpr auto test = [] {
      using original_types = type_set<int, double>;
      using downgraded_types = type_set<int>;

      multitype_memory<dynamic_memory, original_types> original_mem;

      auto downgraded_mem = multitype_memory<dynamic_memory, downgraded_types>(
          original_mem.handler_type::template downgrade<apply_wrapper_t<
              trivial_shared_ptr,
              apply_wrapper_t<dynamic_memory, downgraded_types>>>());

      int *ptr = downgraded_mem.allocate<int>(3);
      bool result = ptr != nullptr;

      downgraded_mem.deallocate(ptr);
      return result;
    };
  };
  expect(test());
};

int main(int, char **) { return 0; }
