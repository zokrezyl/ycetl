#include <boost/ut.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/unordered_set.hpp>

using namespace boost::ut;
using namespace ycetl;

suite unordered_set_suite = [] {
    "insert_and_contains"_test = [] {
        constexpr auto t = [] {
            default_memory<int> mem;
            unordered_set<int> s(mem);
            s.insert(1);
            s.insert(2);
            s.insert(3);
            return s.size() == 3 && s.contains(2) && s.contains(3)
                && !s.contains(99) && !s.empty();
        };
        static_assert(t());
        expect(t());
    };

    "duplicate_insert_no_op"_test = [] {
        constexpr auto t = [] {
            default_memory<int> mem;
            unordered_set<int> s(mem);
            auto [it1, ins1] = s.insert(42);
            auto [it2, ins2] = s.insert(42);
            return ins1 && !ins2 && s.size() == 1;
        };
        static_assert(t());
        expect(t());
    };

    "erase_swap_pop"_test = [] {
        // Order-not-preserved erase: after erasing 2 we should still
        // see exactly {1, 3} (in some order), and contains() should
        // reflect the removal.
        constexpr auto t = [] {
            default_memory<int> mem;
            unordered_set<int> s(mem);
            s.insert(1); s.insert(2); s.insert(3);
            auto n = s.erase(2);
            bool ok = n == 1 && s.size() == 2
                   && s.contains(1) && s.contains(3) && !s.contains(2);
            return ok && s.erase(999) == 0;
        };
        static_assert(t());
        expect(t());
    };

    "iterate"_test = [] {
        constexpr auto t = [] {
            default_memory<int> mem;
            unordered_set<int> s(mem);
            for (int i = 1; i <= 5; ++i) s.insert(i);
            int sum = 0;
            for (auto v : s) sum += v;
            return sum == 15;
        };
        static_assert(t());
        expect(t());
    };
};

int main() {}
