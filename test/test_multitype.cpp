#include <ycetl/multitype.hpp>
#include <ycetl/impl/multitype_storage.hpp>
#include <ycetl/impl/dynamic_storage.hpp>
#include <ycetl/impl/allocator.hpp>

template <typename T>
struct HandlerImpl {
    constexpr T* allocate() const {}
  constexpr T* allocate(std::size_t n) const {}
    constexpr void deallocate(T* p) const {}
    constexpr void deallocate(T* p, std::size_t n) const {}
};

constexpr int test() {
  using working_type_set = ycetl::type_set<int, double, char>;
  auto storage = ycetl::memory::multitype_storage<ycetl::memory::dynamic_storage, working_type_set>();

  auto allocator = ycetl::allocator::allocator<int, decltype(storage)>(storage);

  int *int_ptr = allocator.allocate(10);
  
  
  return 0;
}

int main() {
  using working_type_set = ycetl::type_set<int, double, char>;
  constexpr auto handler1 = ycetl::multitype_handler<HandlerImpl, working_type_set>();

  

}
