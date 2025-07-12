#include <type_traits>
#include <ycetl/type_deduction.hpp>
#include <ycetl/dynamic_array.hpp>

#include <iostream>
#include <string>
#include <typeinfo>
#include <cxxabi.h>


using namespace ycetl;

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


// Base printer
template <typename T>
void print_type(std::ostream& os, int indent = 0) {
    os << std::string(indent, ' ') << "- " << type_name<T>() << "\n";
}

// Specialize for dynamic_array
template <typename T>
void print_type<dynamic_array<T>>(std::ostream& os, int indent) {
    os << std::string(indent, ' ') << "- dynamic_array\n";
    print_type<T>(os, indent + 2);
}

// Specialize for vector
template <typename T>
void print_type<vector<T>>(std::ostream& os, int indent) {
    os << std::string(indent, ' ') << "- vector\n";
    print_type<T>(os, indent + 2);
}

// Specialize for set
template <typename T>
void print_type<set<T>>(std::ostream& os, int indent) {
    os << std::string(indent, ' ') << "- set\n";
    print_type<T>(os, indent + 2);
}

// For type_set<Ts...>
template <typename... Ts>
void print_type_set(std::ostream& os, type_set<Ts...>, int indent = 0) {
    (print_type<Ts>(os, indent), ...);
}


//--- Dummy container types ---//

template <typename T>
struct vector {
    using backend_of = dynamic_array<typename backend_type<T>::type>;
};

template <typename T>
struct set {
    using backend_of = dynamic_array<typename backend_type<T>::type>;
};

//--- Test input and expected output ---//

template <typename... Ts>
using result_set = typename working_type_set<type_set<Ts...>>::type;

// Helpers for checking expected results
template <typename A, typename B>
constexpr bool same = std::is_same_v<A, B>;

int main() {
    // Input: plain + containers + nested + duplicates
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
    >, expected>, "working_type_set failed");


    using result = typename working_type_set<input>::type;

    std::cout << "Resolved working_type_set:\n";
    print_type_set(std::cout, result{});

    return 0;
}

