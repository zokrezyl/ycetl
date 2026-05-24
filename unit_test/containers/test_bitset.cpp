#include <boost/ut.hpp>
#include <ycetl/bitset.hpp>

using namespace boost::ut;
using namespace ycetl;

suite bitset_suite = [] {
  "set_test_reset"_test = [] {
    constexpr auto t = [] {
      bitset<16> b;
      b.set(3);
      b.set(15);
      return b[3] && b[15] && !b[4] && b.count() == 2;
    };
    static_assert(t());
    expect(t());
  };

  "all_none_any"_test = [] {
    constexpr auto t = [] {
      bitset<8> b;
      bool n0 = b.none() && !b.any() && !b.all();
      b.set();
      bool a1 = b.all() && b.any() && !b.none() && b.count() == 8;
      return n0 && a1;
    };
    static_assert(t());
    expect(t());
  };

  "flip_clears_tail_in_partial_word"_test = [] {
    // The last word's high bits are "don't care" — flip() must
    // not surface them via count(). 100 bits = one full 64-bit
    // word + 36 used bits in the second word.
    constexpr auto t = [] {
      bitset<100> b;
      b.flip(); // every legal bit set
      return b.count() == 100 && b.all();
    };
    static_assert(t());
    expect(t());
  };

  "across_word_boundary"_test = [] {
    constexpr auto t = [] {
      bitset<128> b;
      b.set(63);
      b.set(64);
      b.set(127);
      return b[63] && b[64] && b[127] && b.count() == 3;
    };
    static_assert(t());
    expect(t());
  };

  "bitwise_ops"_test = [] {
    constexpr auto t = [] {
      bitset<8> a;
      bitset<8> b;
      a.set(0);
      a.set(2);
      a.set(4); // 0b00010101
      b.set(0);
      b.set(1);
      b.set(2);         // 0b00000111
      auto AND = a & b; // 0b00000101
      auto OR = a | b;  // 0b00010111
      auto XOR = a ^ b; // 0b00010010
      return AND.count() == 2 && AND[0] && AND[2] && OR.count() == 4 && OR[4]
          && XOR.count() == 2 && XOR[1] && XOR[4];
    };
    static_assert(t());
    expect(t());
  };

  "construct_from_word"_test = [] {
    constexpr auto t = [] {
      bitset<8> b(0b10101010);
      return b[1] && b[3] && b[5] && b[7] && !b[0] && !b[2] && b.count() == 4;
    };
    static_assert(t());
    expect(t());
  };

  "equality"_test = [] {
    constexpr auto t = [] {
      bitset<32> a, b;
      a.set(5);
      a.set(10);
      b.set(5);
      b.set(10);
      bool eq = (a == b);
      b.flip(7);
      return eq && !(a == b);
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
