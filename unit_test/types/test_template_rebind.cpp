#include <boost/ut.hpp>
#include <ycetl/trivial_shared_ptr.hpp>
#include <ycetl/type_system.hpp>

using namespace boost::ut;
using namespace ycetl;

// Helpers for testing
struct WithMemory {
  using memory_type = int;
};
struct WithoutMemory {};

// Rebindable types
struct NonRebindable {};
template <typename... Args>
struct Rebindable : template_info<Rebindable, Args...> {};
template <typename... Args>
struct RebindableWithMemory : template_info<RebindableWithMemory, Args...> {
  using memory_type = int;
};

// Test suites

suite has_memory_type_suite = [] {
  "int_has_no_memory_type"_test = [] {
    constexpr auto test = [] { return !has_memory_type_v<int>; };
    static_assert(test());
    expect(test());
  };

  "WithMemory_has_memory_type"_test = [] {
    constexpr auto test = [] { return has_memory_type_v<WithMemory>; };
    static_assert(test());
    expect(test());
  };

  "WithoutMemory_has_no_memory_type"_test = [] {
    constexpr auto test = [] { return !has_memory_type_v<WithoutMemory>; };
    static_assert(test());
    expect(test());
  };
};

suite is_template_rebindable_suite = [] {
  "int_is_not_rebindable"_test = [] {
    constexpr auto test = [] { return !is_template_rebindable_v<int>; };
    static_assert(test());
    expect(test());
  };

  "Rebindable_type_is_rebindable"_test = [] {
    constexpr auto test = [] {
      return is_template_rebindable_v<Rebindable<int>>;
    };
    static_assert(test());
    expect(test());
  };

  "NonRebindable_type_is_not_rebindable"_test = [] {
    constexpr auto test = [] {
      return !is_template_rebindable_v<NonRebindable>;
    };
    static_assert(test());
    expect(test());
  };
};

suite has_rebindable_memory_suite = [] {
  "int_has_no_rebindable_memory"_test = [] {
    constexpr auto test = [] { return !has_rebindable_memory_v<int>; };
    static_assert(test());
    expect(test());
  };

  "WithMemory_but_not_rebindable"_test = [] {
    constexpr auto test = [] { return !has_rebindable_memory_v<WithMemory>; };
    static_assert(test());
    expect(test());
  };

  "Rebindable_but_no_memory"_test = [] {
    constexpr auto test = [] {
      return !has_rebindable_memory_v<Rebindable<int>>;
    };
    static_assert(test());
    expect(test());
  };

  "Rebindable_with_memory"_test = [] {
    constexpr auto test = [] {
      return has_rebindable_memory_v<RebindableWithMemory<int>>;
    };
    static_assert(test());
    expect(test());
  };
};

suite rebind_memory_suite = [] {
  "rebind_memory_non_rebindable_returns_original"_test = [] {
    constexpr auto test = [] {
      return std::is_same_v<rebind_memory_t<int, WithMemory>, int>;
    };
    static_assert(test());
    expect(test());
  };

  "rebind_memory_rebindable_with_memory"_test = [] {
    constexpr auto test = [] {
      using original = RebindableWithMemory<WithoutMemory>;
      using rebound = rebind_memory_t<original, WithMemory>;
      return std::is_same_v<rebound, RebindableWithMemory<WithMemory>>;
    };
    static_assert(test());
    expect(test());
  };
};

template <typename T> struct WrapperA {
  using type = T;
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
      using original = type_set<int, double>;
      using wrapped = apply_wrapper_t<trivial_shared_ptr, original>;
      return std::is_same_v<wrapped, type_set<trivial_shared_ptr<int>,
                                              trivial_shared_ptr<double>>>;
    };
    static_assert(test());
    expect(test());
  };
};

int main() {}
