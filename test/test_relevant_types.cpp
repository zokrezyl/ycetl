#include <type_traits>
#include <ycetl/relevant_types.hpp>
#include <ycetl/dynamic_array.hpp>

#include <iostream>
#include <string>
#include <typeinfo>
#include <cxxabi.h>

using namespace ycetl;

//===----------------------------------------------------------------------===//
// Utilities
//===----------------------------------------------------------------------===//

std::string demangle(const char* name) {
    int status = 0;
    char* realname = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    std::string result = (status == 0 && realname) ? realname : name;
    std::free(realname);
    return result;
}

template <typename T>
std::string type_name() {
    return demangle(typeid(T).name());
}

//===----------------------------------------------------------------------===//
// Dummy container types
//===----------------------------------------------------------------------===//

template <typename T, typename = void>
struct has_storage_type : std::false_type {};

template <typename T>
struct has_storage_type<T, std::void_t<typename T::storage_type>>
    : std::true_type {};

template <typename T, bool = has_storage_type<T>::value>
struct storage_type_trait {
  using type = T;
};

template <typename T> struct storage_type_trait<T, true> {
  using type = typename T::storage_type;
};

template <typename T>
struct vector {
  using storage_unit = typename storage_type_trait<T>::type;
  using storage_type = dynamic_array<storage_unit>;
  using relevant_of =
      type_set_cat_t<storage_unit, typename relevant_types_of<T>::type>;
};


template <typename T>
struct set {
  using storage_unit = typename storage_type_trait<T>::type;
  using storage_type = dynamic_array<storage_unit>;
  using relevant_of =
      type_set_cat_t<storage_type, typename relevant_types_of<T>::type>;
};

//===----------------------------------------------------------------------===//
// Type pretty-printer (for relevant_types results)
//===----------------------------------------------------------------------===//

template <typename T>
void print_type(std::ostream& os, int indent = 0);

template <typename T>
void print_type_impl(std::ostream& os, const dynamic_array<T>&, int indent) {
    os << std::string(indent, ' ') << "- dynamic_array\n";
    print_type<T>(os, indent + 2);
}

template <typename T>
void print_type_impl(std::ostream& os, const vector<T>&, int indent) {
    os << std::string(indent, ' ') << "- vector\n";
    print_type<T>(os, indent + 2);
}

template <typename T>
void print_type_impl(std::ostream& os, const set<T>&, int indent) {
    os << std::string(indent, ' ') << "- set\n";
    print_type<T>(os, indent + 2);
}

template <typename T>
void print_type_impl(std::ostream& os, const T&, int indent) {
    os << std::string(indent, ' ') << "- " << type_name<T>() << "\n";
}

template <typename T>
void print_type(std::ostream& os, int indent) {
    print_type_impl(os, T{}, indent);
}

template <typename... Ts>
void print_type_set(std::ostream& os, type_set<Ts...>, int indent = 0) {
    (print_type<Ts>(os, indent), ...);
}

//===----------------------------------------------------------------------===//
// Test logic
//===----------------------------------------------------------------------===//

template <typename... Ts>
using result_set = relevant_types_t<type_set<Ts...>>;

template <typename A, typename B>
constexpr bool same = std::is_same_v<A, B>;

#if 0
int _main() {
    using input = type_set<
        int,
        vector<int>,
        vector<vector<int>>,
        set<int>,
        set<vector<int>>,
        vector<int> // duplicate
    >;

    using expected = type_set<
        int,
        dynamic_array<int>,
        dynamic_array<dynamic_array<int>>
    >;

    static_assert(same<result_set<
        int,
        vector<int>,
        vector<vector<int>>,
        set<int>,
        set<vector<int>>,
        vector<int>
    >, expected>, "relevant_types failed");

    using result = relevant_types_t<input>;

    std::cout << "Resolved relevant types:\n";
    print_type_set(std::cout, result{});

    return 0;
}
#endif

int main() {
    using input = type_set<vector<vector<int>>>;

    using expected = type_set<
        int,
        dynamic_array<int>>;

    //using result = relevant_types_t<input>;
    using result1 = relevant_types_t<
  vector<vector<int>>,
  set<vector<int>>
  >;
    using result2 = relevant_types_t<
  vector<vector<int>>,
  vector<vector<int>>
  >;

    std::cout << "Resolved relevant types1:\n";
    print_type_set(std::cout, result1{});

    std::cout << "\n\nResolved relevant types2:\n";
    print_type_set(std::cout, result2{});

    std::cout << "\n\nExpected relevant types:\n";
    print_type_set(std::cout, expected{});

    return 0;
}

