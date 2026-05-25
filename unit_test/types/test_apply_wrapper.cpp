// SPDX-License-Identifier: MIT

#include <boost/ut.hpp>
#include <ycetl/trivial_shared_ptr.hpp>
#include <ycetl/type_system.hpp>

using namespace boost::ut;
using namespace ycetl;

template <typename T> struct WrapperA {
  using type = T;
};
template <typename T> struct WrapperB {
  using type = trivial_shared_ptr<T>;
};

struct SimpleType {
  constexpr static int value = 42;
};
struct AnotherType {
  constexpr static int value = 100;
};

suite apply_wrapper_suite = [] {
  "apply_wrapper_identity"_test = [] {
    constexpr auto test = [] {
      using original = type_set<int, char, double>;
      using wrapped = apply_wrapper_t<WrapperA, original>;
      return std::is_same_v<
          wrapped, type_set<WrapperA<int>, WrapperA<char>, WrapperA<double>>>;
    };
    static_assert(test());
    expect(test());
  };

  "apply_wrapper_shared_ptr"_test = [] {
    constexpr auto test = [] {
      using original = type_set<SimpleType, AnotherType>;
      using wrapped = apply_wrapper_t<trivial_shared_ptr, original>;
      return std::is_same_v<wrapped, type_set<trivial_shared_ptr<SimpleType>,
                                              trivial_shared_ptr<AnotherType>>>;
    };
    static_assert(test());
    expect(test());
  };

  "apply_wrapper_custom_wrapper"_test = [] {
    constexpr auto test = [] {
      using original = type_set<int, SimpleType>;
      using wrapped = apply_wrapper_t<WrapperB, original>;
      return std::is_same_v<wrapped,
                            type_set<WrapperB<int>, WrapperB<SimpleType>>>;
    };
    static_assert(test());
    expect(test());
  };
};

int main(int argc, char **argv) { return 0; }
