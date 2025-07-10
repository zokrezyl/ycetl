

#include <iostream>
#include <type_traits>
#include <stdexcept>


struct A{
  int i;
  int j;

};

struct B{
  int i;
  int j;
  A a;
};



constexpr int test( ) {
    // Example usage
    multi_storage<int, double, A, B> a;
    
    // Set values
    a.get<int>() = 42;
    a.get<double>() = 3.14;
  


  
  return 0;
};

int main() {

  constexpr int result = test();

}
