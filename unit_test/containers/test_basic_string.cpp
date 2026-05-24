#include <boost/ut.hpp>
#include <string_view>
#include <ycetl/basic_string.hpp>
#include <ycetl/string.hpp>

using namespace boost::ut;
using namespace ycetl;

suite basic_string_suite = [] {
    "default_ctor_empty"_test = [] {
        constexpr auto t = [] {
            string s;
            return s.empty() && s.size() == 0;
        };
        static_assert(t());
        expect(t());
    };

    "construct_from_literal"_test = [] {
        constexpr auto t = [] {
            string s = "hello";
            return s.size() == 5 && s[0] == 'h' && s.back() == 'o';
        };
        static_assert(t());
        expect(t());
    };

    "append_and_concat"_test = [] {
        constexpr auto t = [] {
            string s = "hello";
            s += ", ";
            s += "world";
            return s.size() == 12 && s.view() == std::string_view("hello, world");
        };
        static_assert(t());
        expect(t());
    };

    "find_substring"_test = [] {
        constexpr auto t = [] {
            string s = "the quick brown fox";
            return s.find('q')                    == 4
                && s.find(std::string_view("fox")) == 16
                && s.find(std::string_view("cat")) == string::npos
                && s.find('x', 17)                == 18;
        };
        static_assert(t());
        expect(t());
    };

    "substr"_test = [] {
        constexpr auto t = [] {
            string s = "abcdef";
            auto mid = s.substr(2, 3);                // "cde"
            return mid.size() == 3 && mid.view() == std::string_view("cde");
        };
        static_assert(t());
        expect(t());
    };

    "iteration"_test = [] {
        constexpr auto t = [] {
            string s = "abc";
            int seen = 0;
            for (auto c : s) seen = seen * 256 + c;
            return seen == ('a' * 256 + 'b') * 256 + 'c';
        };
        static_assert(t());
        expect(t());
    };

    "equality"_test = [] {
        constexpr auto t = [] {
            string a = "test";
            string b = "test";
            string c = "tess";
            return (a == b) && !(a == c) && a == std::string_view("test");
        };
        static_assert(t());
        expect(t());
    };
};

int main() {}
