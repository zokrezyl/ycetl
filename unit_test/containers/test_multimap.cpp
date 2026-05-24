#include <boost/ut.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/multimap.hpp>

using namespace boost::ut;
using namespace ycetl;

suite multimap_suite = [] {
    "insert_allows_duplicate_keys"_test = [] {
        constexpr auto t = [] {
            default_memory<std::pair<int, int>> mem;
            multimap<int, int> m(mem);
            m.insert({1, 10});
            m.insert({1, 20});
            m.insert({1, 30});
            m.insert({2, 200});
            return m.size() == 4 && m.count(1) == 3 && m.count(2) == 1;
        };
        static_assert(t());
        expect(t());
    };

    "insertion_order_preserved_within_key"_test = [] {
        constexpr auto t = [] {
            default_memory<std::pair<int, int>> mem;
            multimap<int, int> m(mem);
            m.insert({5, 1});
            m.insert({5, 2});
            m.insert({5, 3});
            auto [lo, hi] = m.equal_range(5);
            int expected = 1;
            for (auto it = lo; it != hi; ++it) {
                if (it->second != expected) return false;
                ++expected;
            }
            return expected == 4;
        };
        static_assert(t());
        expect(t());
    };

    "sorted_by_key"_test = [] {
        constexpr auto t = [] {
            default_memory<std::pair<int, int>> mem;
            multimap<int, int> m(mem);
            for (int k : {3, 1, 3, 2, 1, 3}) m.insert({k, k * 10});
            int prev = 0;
            for (auto &p : m) { if (p.first < prev) return false; prev = p.first; }
            return m.size() == 6 && m.count(3) == 3 && m.count(1) == 2;
        };
        static_assert(t());
        expect(t());
    };

    "erase_removes_all_matching"_test = [] {
        constexpr auto t = [] {
            default_memory<std::pair<int, int>> mem;
            multimap<int, int> m(mem);
            m.insert({1, 100});
            m.insert({2, 200});
            m.insert({2, 201});
            m.insert({3, 300});
            return m.erase(2) == 2 && m.size() == 2 && !m.contains(2)
                && m.find(1)->second == 100 && m.find(3)->second == 300;
        };
        static_assert(t());
        expect(t());
    };
};

int main() {}
