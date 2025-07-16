#include <ycetl/vector.hpp>
#include <ycetl/impl/multitype_memory.hpp>
#include <ycetl/impl/dynamic_memory.hpp>
#include <ycetl/impl/allocator.hpp>
#include <ycetl/impl/type_printer.hpp>
#include <ycetl/type_id.hpp>
//#include <ycetl/dynamic_allocator.hpp>
#include <iostream>

using namespace ycetl;

struct Test {
  int i;
  int j;
};

constexpr auto get_vector_0()  {
  using vector_t = ycetl::vector<int>;
  vector_t v1;
  v1.push_back(1);
  v1.push_back(2);
  return v1;
}

constexpr auto get_vector_1()  {
  using vector_t = ycetl::vector<ycetl::vector<int>>;
  vector_t v1;
  return v1;
}


constexpr auto get_vector_111()  {
  using vector_t = ycetl::vector<ycetl::vector<int>>;

  using relevant_types =
      ycetl::relevant_types_t<vector_t>;

    using memory_t = ycetl::default_memory<relevant_types>;
}

constexpr auto get_vector_2()  {
  using vector_t = ycetl::vector<ycetl::vector<int>>;

  vector_t v1;
  return v1;
}

constexpr auto test_1111() {
  auto result = get_vector_1();
  return ycetl::type_id::type_id<decltype(result)::memory_type>();
}

constexpr int test_13 (){
  auto allocator = default_memory<relevant_types_t<vector<Test>, vector<int>>>();

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


constexpr int test_00() {
  auto result = get_vector_0();
  //static_assert(result == 0, "Test failed");
  //test_01();
  return result.size();
}

constexpr auto test_01() {
  auto result = get_vector_0();
  return ycetl::type_id::type_id<decltype(result)::memory_type>();
}

constexpr auto test_11() {
  auto result = get_vector_1();
  //constexpr auto v1 = get_vector_1();
  //constexpr auto v2 = get_vector_2();
  //static_assert(v1.size() == 0, "v1 should be empty");
  //static_assert(v2.size() == 0, "v2 should be empty");
  return ycetl::type_id::type_id<decltype(result)::memory_type>();
}


int main() {
  constexpr auto result_1111 = test_1111();
  constexpr auto result_00 = test_00();
  static_assert(result_00 == 2, "Test 0 failed");
  constexpr auto result_01 = test_01();
  std::cout << "Type ID of result_01: " << result_01 << std::endl;

  constexpr auto result_11 = test_11();
  std::cout << "Type ID of result_11: " << result_11 << std::endl;


  return 0;
}
