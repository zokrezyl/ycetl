#include <boost/ut.hpp>
#include <ycetl/hive.hpp>

using namespace boost::ut;
using namespace ycetl;

suite hive_suite = [] {
    "insert_iterate"_test = [] {
        constexpr auto t = [] {
            hive<int> h;
            for (int i = 1; i <= 5; ++i) h.insert(i);
            int sum = 0;
            for (auto v : h) sum += v;
            return h.size() == 5 && sum == 15 && h.capacity() == 5;
        };
        static_assert(t());
        expect(t());
    };

    "erase_recycles_slot"_test = [] {
        // Insert 5, erase the middle one — capacity must stay at 5,
        // and the next insert must reuse the freed slot rather than
        // grow the storage.
        constexpr auto t = [] {
            hive<int> h;
            auto it1 = h.insert(10);
            auto it2 = h.insert(20);
            auto it3 = h.insert(30);
            (void)it1; (void)it3;
            h.erase(it2);

            bool ok = h.size() == 2 && h.capacity() == 3;
            h.insert(99);
            // The new 99 should land in the freed middle slot; capacity
            // stays at 3 — the load-bearing free-list reuse property.
            return ok && h.size() == 3 && h.capacity() == 3;
        };
        static_assert(t());
        expect(t());
    };

    "iteration_skips_erased"_test = [] {
        constexpr auto t = [] {
            hive<int> h;
            for (int i = 1; i <= 6; ++i) h.insert(i);
            // Erase the even values via a fresh probe each time
            // (iterators stay valid across erase, but the simplest
            // proof is "the rest of the container is still walkable").
            for (auto it = h.begin(); it != h.end(); ) {
                if (*it % 2 == 0) it = h.erase(it);
                else              ++it;
            }
            int sum = 0;
            for (auto v : h) sum += v;
            return h.size() == 3 && sum == (1 + 3 + 5);
        };
        static_assert(t());
        expect(t());
    };

    "reserve_seeds_free_list"_test = [] {
        // reserve() doesn't just allocate raw capacity — it seeds the
        // free-list so the first N inserts hit O(1) into pre-reserved
        // slots, without growing the underlying storage.
        constexpr auto t = [] {
            hive<int> h;
            h.reserve(8);
            bool ok = h.empty() && h.capacity() == 8;
            for (int i = 0; i < 8; ++i) h.insert(i * 10);
            return ok && h.size() == 8 && h.capacity() == 8;
        };
        static_assert(t());
        expect(t());
    };

    "clear_resets_everything"_test = [] {
        constexpr auto t = [] {
            hive<int> h;
            for (int i = 0; i < 10; ++i) h.insert(i);
            h.clear();
            bool empty = h.empty() && h.size() == 0 && h.capacity() == 0;
            h.insert(42);
            return empty && h.size() == 1;
        };
        static_assert(t());
        expect(t());
    };

    "stable_iterator_value_survives_unrelated_erase"_test = [] {
        // Inserting and then erasing some OTHER slot must not disturb
        // the value at a held iterator. This is the "P0447 stability"
        // contract, modulo the growth caveat (which we sidestep here
        // by reserving up front).
        constexpr auto t = [] {
            hive<int> h;
            h.reserve(16);
            auto a = h.insert(100);
            auto b = h.insert(200);
            auto c = h.insert(300);
            (void)a; (void)c;
            h.erase(b);          // unrelated erase
            return *a == 100 && *c == 300 && h.size() == 2;
        };
        static_assert(t());
        expect(t());
    };
};

int main() {}
