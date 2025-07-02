// #include <ycetl/memory.hpp>
// clang-format off
#include <ycetl/memory.hpp>
#include <ycetl/trivial_array.hpp>
//#include <ycetl/misc.hpp>
#include <iostream>
#include <memory>

//ycetl::rom_arena<MN(100)> rom_arena;
//
#include <array>

template <typename T, std::size_t N>
class my_array {
  T  _data[N];
public:
  constexpr my_array():_data() {}
};

template <std::size_t N>
struct my_array2 {
  ycetl::trivial_array<unsigned char, N> _data;
  constexpr my_array2() : _data() {
  }
};

//#define SIZE (800 * 1024 * 1024)
#define SIZE (8 * 1024 * 1024)

//constexpr unsigned char _data[SIZE];
template <std::size_t _SIZE_>
class Test {
public:
  ycetl::rom_arena<_SIZE_> _rom_arena;
  ycetl::_Allocator __allocator;
  ycetl::Allocator _allocator;
  constexpr Test(): _rom_arena(), 
    __allocator(_rom_arena.memory(), _rom_arena.size()),
    _allocator(&__allocator)
  {
  }
  constexpr ycetl::size_t size() const {
    return _rom_arena.size();
  }
};

//constexpr my_array2<SIZE> rom_arena3;
//constexpr ycetl::trivial_array<unsigned char, SIZE> _data;
//constexpr my_array<unsigned char, SIZE> rom_arena;
constexpr Test<SIZE> t;

//constexpr std::array<unsigned char, SIZE> rom_arena{};
//Test<SIZE> rom_arena{};


int main() {
  //static_assert(rom_arena.size() == SIZE, "Size mismatch");

  std::cout << "Size of rom_arena: " << t.size() / (1024 * 1024)
            << " Mbytes\n";
  /*
  static_assert(sizeof(rom_arena) == SIZE, "Size mismatch");
  int size_in_mb = sizeof(rom_arena) / (1024 * 1024);
  int size_orig = SIZE / (1024 * 1024);
  std::cout << "Size of rom_arena: " << size_in_mb << " Mbytes\n";
  std::cout << "Size of rom_arena: " << size_orig << " Mbytes\n";
  */

}

// clang-format on
