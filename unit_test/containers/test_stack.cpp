#include <boost/ut.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/stack.hpp>

using namespace boost::ut;
using namespace ycetl;

suite stack_suite = [] {
    "push_top_pop"_test = [] {
        constexpr auto t = [] {
            default_memory<int> mem;
            stack<int> s(mem);
            s.push(1);
            s.push(2);
            s.push(3);
            bool ok = s.size() == 3 && s.top() == 3 && !s.empty();
            s.pop();
            ok = ok && s.top() == 2 && s.size() == 2;
            s.pop();
            s.pop();
            return ok && s.empty();
        };
        static_assert(t());
        expect(t());
    };

    "lifo_order"_test = [] {
        constexpr auto t = [] {
            default_memory<int> mem;
            stack<int> s(mem);
            for (int i = 1; i <= 5; ++i) s.push(i);
            int sum = 0;
            int peek_first = s.top();
            while (!s.empty()) { sum += s.top(); s.pop(); }
            return peek_first == 5 && sum == 15;
        };
        static_assert(t());
        expect(t());
    };

    "emplace"_test = [] {
        constexpr auto t = [] {
            default_memory<int> mem;
            stack<int> s(mem);
            s.emplace(42);
            s.emplace(7);
            return s.size() == 2 && s.top() == 7;
        };
        static_assert(t());
        expect(t());
    };
};

int main() {}
