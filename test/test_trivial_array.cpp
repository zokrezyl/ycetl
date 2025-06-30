#include <ycetl/trivial_array.hpp>
#include <ycetl/misc.hpp>
#include <iostream>
#include <array>

ycetl::trivial_array<int, 5> arr;

constexpr int test1() {
  auto a = GN(5);
  ycetl::trivial_array<int, GN(500)> arr;
  return 0;
}

constexpr int test2() {
  ycetl::trivial_array<int, 5> arr;
  for (int i = 0; i < 5; ++i) {
    arr[i] = i * 10;
  }
  return arr[2]; // should return 20
}

constexpr long long unsigned int test3() {
  //std::array<int, 1024 * 1014 * 1024> t;
  int I[1024 * 1024 * 1]; 
  return sizeof(I); // should return 1024 * 1024 * 1024
}

constexpr int test() {
  test1();
  test2();
  test3();
  return 0;
}

constexpr long long unsigned int i = test();

int main() {

  //constexpr std::array<int, GN(5)> t;

  static_assert(i == 0, "Test failed");

  std::cout << "Test result: " << i << std::endl;

  std::cout << GN(5) << std::endl;

}
