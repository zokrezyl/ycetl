// test_object_pool.cpp
#include <iostream>          // guard iostreams
#include <boost/ut.hpp>
#include <ycetl/object_pool.hpp>   // defines object_pool / dynamic_object_pool

using namespace boost::ut;
using ycetl::object_pool;
using ycetl::dynamic_object_pool;

/*──────────────────────── object_pool tests ─────────────────────*/
suite object_pool_suite = [] {


  "empty_pool"_test = [] {
    constexpr auto test = []() {
    object_pool<int> p(8);         // capacity 8, nothing yet
    expect(p.size() == 0_u);
    expect(!p.is_full());
  };
  };

  "fill_exact_capacity"_test = [] {
    object_pool<int> p(4);
    for (int i = 0; i < 4; ++i) p.add(i);
    expect(p.size() == 4_u);
    expect(p.is_full());           // should report full now
  };

  "fill_100"_test = [] {
    object_pool<int> p(100);
    for (int i = 0; i < 100; ++i) p.add(i);
    expect(p.size() == 100_u);
    expect(p.data()[99] == 99_i);  // verify last written value
  };
};

/* simple main for Boost.UT */
int main(int argc, char** argv) {
}

