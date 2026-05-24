#include <boost/ut.hpp>
#include <ycetl/map.hpp>
#include <ycetl/memory.hpp>

using namespace boost::ut;
using namespace ycetl;

suite map_suite = [] {
    "insert_and_find"_test = [] {
        constexpr auto t = [] {
            default_memory<std::pair<int, int>> mem;
            map<int, int> m(mem);
            m.insert({3, 30});
            m.insert({1, 10});
            m.insert({2, 20});
            auto it = m.find(2);
            bool ok = m.size() == 3 && it != m.end() && it->second == 20
                   && m.find(99) == m.end();
            return ok;
        };
        static_assert(t());
        expect(t());
    };

    "sorted_order"_test = [] {
        // Flat sorted map's defining invariant — keys come out in
        // ascending order regardless of insertion order.
        constexpr auto t = [] {
            default_memory<std::pair<int, int>> mem;
            map<int, int> m(mem);
            for (int k : {5, 2, 8, 1, 9, 3}) m.insert({k, k * 10});

            int prev = -1;
            bool ok = true;
            for (auto &p : m) {
                ok = ok && (p.first > prev);
                prev = p.first;
            }
            return ok && prev == 9 && m.size() == 6;
        };
        static_assert(t());
        expect(t());
    };

    "operator_bracket_default_inserts"_test = [] {
        constexpr auto t = [] {
            default_memory<std::pair<int, int>> mem;
            map<int, int> m(mem);
            int &v = m[42];      // inserts default int{} == 0
            bool ok = m.size() == 1 && v == 0;
            m[42] = 7;
            return ok && m[42] == 7 && m.size() == 1;
        };
        static_assert(t());
        expect(t());
    };

    "insert_or_assign"_test = [] {
        constexpr auto t = [] {
            default_memory<std::pair<int, int>> mem;
            map<int, int> m(mem);
            auto [it1, ins1] = m.insert_or_assign(1, 100);
            auto [it2, ins2] = m.insert_or_assign(1, 200);
            return ins1 && !ins2 && m.find(1)->second == 200 && m.size() == 1;
        };
        static_assert(t());
        expect(t());
    };

    "erase_preserves_order"_test = [] {
        constexpr auto t = [] {
            default_memory<std::pair<int, int>> mem;
            map<int, int> m(mem);
            for (int k : {1, 2, 3, 4, 5}) m.insert({k, k});
            auto n = m.erase(3);
            int prev = 0;
            bool ok = n == 1 && m.size() == 4 && !m.contains(3);
            for (auto &p : m) { ok = ok && p.first > prev; prev = p.first; }
            return ok && m.erase(999) == 0;
        };
        static_assert(t());
        expect(t());
    };
};

int main() {}
