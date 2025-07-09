#include <ycetl/dynamic_storage.hpp>
#include <ycetl/static_storage.hpp>
#include <ycetl/factory.hpp>
#include <ycetl/vector.hpp>

#include <vector>


struct Test {
  int i;

};

template<class Tp>
struct NAlloc
{
    std::array<Tp, 1024 * 1024> _buffer; // 1MB buffer for allocation
    std::size_t _pos = 0; // Current size of the buffer 
    typedef Tp value_type;
 
    NAlloc() = default;
    template<class T>
    NAlloc(const NAlloc<T>&) {}
 
    constexpr Tp* allocate(std::size_t n)
    {
        //Tp* p = static_cast<Tp*>(::operator new(n));
        //std::cout << "allocating " << n << " bytes @ " << p << '\n';
        Tp * p = _buffer.data() + _pos;
        _pos ++;
        return p;
    }
    constexpr void deallocate(Tp* p, std::size_t n)
    {
        //std::cout << "deallocating " << n * sizeof *p << " bytes @ " << p << "\n\n";
        //::operator delete(p);
    }
};

template<class T, class U>
bool operator==(const NAlloc<T>&, const NAlloc<U>&) { return true; }
 
template<class T, class U>
bool operator!=(const NAlloc<T>&, const NAlloc<U>&) { return false; }

constexpr int max_elements = 32;

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

constexpr int test_vector(){
    ycetl::vector<int> v1;
    //std::vector<int> v2(allocator);

    //v1.reserve(max_elements); // reserves at least max_elements * sizeof(int) bytes

    for (int n = 0; n < max_elements; ++n) 
        v1.push_back(n);
    return v1.size();
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
    
    constexpr int res = test_vector();
    static_assert(res == max_elements, "Vector test failed");
    
    return 0; 
}
