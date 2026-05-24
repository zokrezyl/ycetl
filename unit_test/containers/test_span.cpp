#include <array>

#include <boost/ut.hpp>
#include <ycetl/span.hpp>

using namespace boost::ut;
using namespace ycetl;

suite span_suite = [] {
    "empty"_test = [] {
        constexpr auto t = [] {
            span<int> s;
            return s.empty() && s.size() == 0_u && s.data() == nullptr;
        };
        static_assert(t());
        expect(t());
    };

    "from_c_array"_test = [] {
        constexpr auto t = [] {
            int arr[]{1, 2, 3, 4, 5};
            span<int> s(arr);
            return s.size() == 5_u && s.front() == 1 && s.back() == 5
                && s[2] == 3 && !s.empty() && s.size_bytes() == 5 * sizeof(int);
        };
        static_assert(t());
        expect(t());
    };

    "from_std_array"_test = [] {
        constexpr auto t = [] {
            std::array<int, 3> a{7, 8, 9};
            span<int> s(a);
            int sum = 0;
            for (auto v : s) sum += v;
            return sum == 24 && s[1] == 8;
        };
        static_assert(t());
        expect(t());
    };

    "ptr_size"_test = [] {
        constexpr auto t = [] {
            int arr[]{10, 20, 30};
            span<int> s(arr, 3);
            return *s.begin() == 10 && *(s.end() - 1) == 30;
        };
        static_assert(t());
        expect(t());
    };

    "subspan"_test = [] {
        constexpr auto t = [] {
            int arr[]{0, 1, 2, 3, 4, 5};
            span<int> s(arr);
            auto head = s.first(2);
            auto tail = s.last(2);
            auto mid  = s.subspan(1, 3);
            return head.size() == 2_u && head[1] == 1
                && tail.size() == 2_u && tail[0] == 4 && tail[1] == 5
                && mid.size()  == 3_u && mid[0] == 1 && mid[2] == 3;
        };
        static_assert(t());
        expect(t());
    };

    "const_view"_test = [] {
        constexpr auto t = [] {
            const std::array<int, 4> a{2, 4, 6, 8};
            span<const int> s(a);
            int sum = 0;
            for (auto v : s) sum += v;
            return sum == 20;
        };
        static_assert(t());
        expect(t());
    };
};

int main() {}
