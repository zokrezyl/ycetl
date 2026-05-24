#include <boost/ut.hpp>
#include <ycetl/forward_list.hpp>

using namespace boost::ut;
using namespace ycetl;

suite forward_list_suite = [] {
    "push_front_and_iterate"_test = [] {
        constexpr auto t = [] {
            forward_list<int> l;
            for (int i = 1; i <= 4; ++i) l.push_front(i);  // 4, 3, 2, 1
            int seen[4]{};
            int idx = 0;
            for (auto v : l) seen[idx++] = v;
            return l.size() == 4 && idx == 4
                && seen[0] == 4 && seen[1] == 3
                && seen[2] == 2 && seen[3] == 1;
        };
        static_assert(t());
        expect(t());
    };

    "init_list_preserves_order"_test = [] {
        constexpr auto t = [] {
            forward_list<int> l{10, 20, 30};
            return l.front() == 10 && l.size() == 3;
        };
        static_assert(t());
        expect(t());
    };

    "insert_after"_test = [] {
        constexpr auto t = [] {
            forward_list<int> l{1, 2, 4};
            auto it = l.begin(); ++it;          // points at 2
            l.insert_after(it, 3);
            int seen[4]{};
            int idx = 0;
            for (auto v : l) seen[idx++] = v;
            return l.size() == 4 && seen[0] == 1 && seen[1] == 2
                && seen[2] == 3 && seen[3] == 4;
        };
        static_assert(t());
        expect(t());
    };

    "erase_after"_test = [] {
        constexpr auto t = [] {
            forward_list<int> l{1, 2, 3, 4};
            auto it = l.begin();                 // points at 1
            l.erase_after(it);                   // removes 2
            int seen[3]{};
            int idx = 0;
            for (auto v : l) seen[idx++] = v;
            return l.size() == 3 && seen[0] == 1 && seen[1] == 3 && seen[2] == 4;
        };
        static_assert(t());
        expect(t());
    };

    "pop_front"_test = [] {
        constexpr auto t = [] {
            forward_list<int> l{1, 2, 3};
            l.pop_front();
            return l.size() == 2 && l.front() == 2;
        };
        static_assert(t());
        expect(t());
    };
};

int main() {}
