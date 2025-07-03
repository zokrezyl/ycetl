#include <ycetl/vector.hpp>
#include <ycetl/dynamic_allocator.hpp>
#include <vector>


#include <memory>

template <typename _Tp, typename _Alloc> struct test {

  typedef typename std::allocator_traits<_Alloc>::template rebind_alloc<_Tp> _Tp_alloc_type;

};


int main() {

  std::vector<int> std_vector;
  ycetl::vector<int> ycetl_vector;

  ycetl_vector.push_back(1);
}
