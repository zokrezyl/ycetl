#include <ycetl/memory.hpp>
#include <iostream>

class Test {
  ycetl::Allocator _allocator;
public:
  Test(ycetl::Allocator allocator): _allocator(allocator) {
    // Constructor logic, if needed

  }
};


constexpr int test() {
  ycetl::Allocator allocator;
  allocator.allocate(100); // allocate 100 bytes
  return  0;
}


int main() {
  //constexpr ycetl::AllocatorPtr ptr = test();
  //constexpr auto allocator = ptr.allocator();
  /*
  if (constexpr ptr.allocator() != nullptr) {

    return ptr; // or handle the error as needed
  }

  std::cout << "AllocatorPtr test passed!" << std::endl;
  std::cout << "AllocatorPtr size: " << ptr.allocator() << std::endl;
  */
  return 0; // success
}
