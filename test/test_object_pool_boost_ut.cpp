// file: test_object_pool.cpp
//
// compile with:  g++ -std=c++23 -I/path/to/boost ut test_object_pool.cpp
// or (MSVC)     cl /std:c++23 /I path\to\boost ut test_object_pool.cpp
//
// clang-format: on

#include <boost/ut.hpp>
#include <ycetl/trace.hpp>   // your header

using namespace boost::ut;


namespace ycetl::trace {

struct pool_test_tag {
    template<class P> static std::size_t global_slots(const P& p)   { return p._global_slots_taken; }
    template<class P> static std::size_t pools_allocated(const P& p){ return p._pools_allocated; }
    template<class P> static std::size_t pool_size    (const P& p)  { return p._pool_size; }

    template<typename T>
    static T** pool_vector(const object_pool<T>& p)         { return p._pool_vector; }


    template<class P>
    static const auto& elem(const P& p, std::size_t blk, std::size_t off)
    { return p._pool_vector[blk][off]; }
};

} // namespace ycetl::trace


using namespace ycetl::trace;

/* ---------------------------------------------------------------------
   Helper: create a pool, push N ints, return pointer to first element.
   Works in both constexpr and run-time contexts.
------------------------------------------------------------------------*/
using namespace ycetl::trace;

template<std::size_t N>
constexpr const int* build_pool(object_pool<int>& pool)
{
    const int* first = nullptr;

    for (std::size_t i = 0; i < N; ++i) {
        std::size_t idx = pool.add(static_cast<int>(i));
        if (i == 0)  {                      // need pointer to element 0
            auto blk  = idx / pool_test_tag::pool_size(pool);
            auto off  = idx % pool_test_tag::pool_size(pool);
            first = pool_test_tag::pool_vector(pool)[blk] + off;
        }
    }
    return first;
}


/* ---------------------------------------------------------------------
   Tests
------------------------------------------------------------------------*/

suite object_pool_suite = [] {

  "empty_pool"_test = [] {
    constexpr object_pool<int> p(1000);
    expect(pool_test_tag::global_slots(p) == 0_u);   // runtime check OK
  };

  "constexpr_build"_test = [] {
    object_pool<int> p(1000);
    build_pool<15>(p);
    expect(pool_test_tag::global_slots(p) == 15_u);  // runtime check
  };


    "allocate_single_block"_test = [] {
        object_pool<int> p(1000);
        expect(0_u == pool_test_tag::global_slots(p));

        p.add(42);

        expect(1_u == pool_test_tag::global_slots(p));
        expect(42_i == pool_test_tag::elem(p, 0, 0));
    };

    "cross_block_boundary"_test = [] {
        const std::size_t block = pool_test_tag::pool_size(object_pool<int>{});
        object_pool<int> p;

        for (int i = 0; i < static_cast<int>(block); ++i)
            p.add(i);                          // fills first pool

        p.add(99);                             // should start second pool

        expect(eq(2_u, pool_test_tag::pools_allocated(p)));
        expect( pool_test_tag::elem(p, 1, 0) == 99_i );
    };
    "constexpr_build"_test = [] {
        object_pool<int> p;
        build_pool<15>(p);
        expect(pool_test_tag::global_slots(p) == 15_u);
    };
    /* push 100 objects, verify counters and last value */
    "fill_100"_test = [] {
        object_pool<int> p;
        for (int i = 0; i < 100; ++i) p.add(i);

        expect(pool_test_tag::global_slots(p) == 100_u);
        expect(pool_test_tag::elem(p, 0, 99) == 99_i);   // last in first pool
    };

    /* push 10 000 objects, check total and very last slot */
    "fill_10000"_test = [] {
        object_pool<int> p;
        for (int i = 0; i < 10'000; ++i) p.add(i);

        expect(pool_test_tag::global_slots(p) == 10'000_u);

        const std::size_t ps  = pool_test_tag::pool_size(p);
        const std::size_t blk = 9'999 / ps;
        const std::size_t off = 9'999 % ps;
        expect(pool_test_tag::elem(p, blk, off) == 9'999_i);  // verify last element
    };

};


int main() { }


