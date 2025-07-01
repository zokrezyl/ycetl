// file: test_object_pool.cpp
//
// compile with:  g++ -std=c++23 -I/path/to/boost ut test_object_pool.cpp
// or (MSVC)     cl /std:c++23 /I path\to\boost ut test_object_pool.cpp

#include <boost/ut.hpp>
#include <ycetl/trace.hpp>   // your header

using namespace boost::ut;

using namespace ycetl::trace;

/* ---------------------------------------------------------------------
   Helper: create a pool, push N ints, return pointer to first element.
   Works in both constexpr and run-time contexts.
------------------------------------------------------------------------*/
template<std::size_t N>
constexpr int const* build_pool(object_pool<int>& pool)
{
    int const* first = nullptr;
    for (std::size_t i = 0; i < N; ++i)
        if (i == 0) first = &pool.add(static_cast<int>(i))[0];
        else        pool.add(static_cast<int>(i));
    return first;
}

/* ---------------------------------------------------------------------
   Tests
------------------------------------------------------------------------*/
suite object_pool_suite = [] {

    "empty_pool"_test = [] {
        constexpr object_pool<int> p;
        static_assert(p._global_slots_taken == 0u);
    };

    "allocate_single_block"_test = [] {
        object_pool<int> p;
        expect(0_u == p._global_slots_taken);
        p.add(42);
        expect(1_u == p._global_slots_taken);
        expect(42_i == p._pool_vector[0][0]);
    };

    "cross_block_boundary"_test = [] {
        constexpr std::size_t block = 10;               // matches ctor default
        object_pool<int> p;
        /* fill exactly one block */
        for (int i = 0; i < static_cast<int>(block); ++i) p.add(i);
        /* next insert should force new block */
        p.add(99);

        expect(eq(2_u, p._pools_allocated));            // 2 blocks
        expect( p._pool_vector[1][0] == 99);
    };

    "constexpr_build"_test = [] {
        constexpr auto result = [] {
            object_pool<int> p;
            build_pool<15>(p);                          // crosses block
            return p._global_slots_taken;
        }();
        static_assert(result == 15u);
    };
};

