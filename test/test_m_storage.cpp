#include <ycetl/dynamic_storage.hpp>
#include <ycetl/static_storage.hpp>
#include <ycetl/factory.hpp>


struct Test {
  int i;

};

/*
constexpr Test test_dynamic_storage() {
  using dynamic_storage_type = ycetl::storage::dynamic_storage<Test>;
  dynamic_storage_type dynamic_storage;
  // Test adding an object
  Test *testObj = dynamic_storage.allocate();

  auto storage = ycetl::storage::storage(&dynamic_storage);

  Test *testObj2 = storage.allocate();

  return *testObj2;
};

*/


constexpr Test test_static_storage() {
  
  using static_storage_type = ycetl::storage::static_storage<Test>;

  static_storage_type static_storage;


  ycetl::storage::factory factory(&static_storage);

  auto test_obj =  factory.create<Test>(); // Allocate some space for int
  return *test_obj;

}

int main() {

    std::array<Test, 1024 * 1024> _buffer;

    using static_storage_type = ycetl::storage::static_storage<Test>;

    //auto storage = ycetl::storage::static_storage(_buffer.data(), _buffer.size());
    auto storage = ycetl::storage::static_storage(_buffer);

    ycetl::storage::factory factory(&storage);

    auto test_obj =  factory.create<Test>(); // Allocate some space for int
  //
    test_obj->i = 42;
    //Test t1 = test_dynamic_storage();

    //Test t2 = test_static_storage();
    return 0; 
}
