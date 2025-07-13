#include <ycetl/vector.hpp>
#include <ycetl/impl/multitype.hpp>
#include <ycetl/impl/multitype_allocator.hpp>
#include <ycetl/impl/dynamic_allocator.hpp>
#include <ycetl/impl/allocator.hpp>
//#include <ycetl/dynamic_allocator.hpp>
#include <vector>

using namespace ycetl;

struct Test {
  int i;
  int j;
};

constexpr int test (){

  auto allocator = default_allocator<relevant_types_t<vector<Test>, vector<int>>>();

  using allocator_type = decltype(allocator);

  ycetl::vector<int, allocator_type> ycetl_int_vector(allocator);
  ycetl_int_vector.push_back(42);
  ycetl_int_vector.push_back(100);
  ycetl_int_vector.clear();
  ycetl::vector<Test, allocator_type> ycetl_test_vector(allocator);
  ycetl_test_vector.push_back({1, 2});
  ycetl_test_vector.push_back({3, 4});
  ycetl_test_vector.clear();
  return 0;
}


int main() {

  constexpr int result = test();
  static_assert(result == 0, "Test failed");

  // Additional runtime test
  test();

  return 0;
}
