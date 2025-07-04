//#include <ycetl/vector.hpp>
//#include <ycetl/dynamic_allocator.hpp>
#include <vector>


#include <memory>

template <typename _Tp, typename _Alloc> struct test {

  typedef typename std::allocator_traits<_Alloc>::template rebind_alloc<_Tp> _Tp_alloc_type;

};


struct Test {

  int i;
  int j;
};



int main() {
  std::vector<int> std_int_vector;

  /*
  ycetl::vector<int> ycetl_int_vector;
  ycetl_int_vector.push_back(42);
  ycetl_int_vector.push_back(100);
  ycetl_int_vector.clear();

  ycetl::vector<Test> ycetl_test_vector;
  ycetl_test_vector.push_back({1, 2});
  ycetl_test_vector.push_back({3, 4});
  ycetl_test_vector.clear();
  */
  std::shared_ptr<Test> ptr = std::make_shared<Test>();


}
