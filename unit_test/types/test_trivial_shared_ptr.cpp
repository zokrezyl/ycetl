#include <boost/ut.hpp>
#include <ycetl/trivial_shared_ptr.hpp>

using namespace boost::ut;
using namespace ycetl;

struct SimpleType {
  int *value{};
  constexpr SimpleType(int v = 0) : value(new int(v)) {}
  constexpr ~SimpleType() { delete value; }

  constexpr int get() const { return *value; }
};

struct ComposedType {
  SimpleType *simple{};
  constexpr ComposedType(int v = 0) : simple(new SimpleType(v)) {}
  constexpr ~ComposedType() { delete simple; }

  constexpr int get() const { return simple->get(); }
};

struct NestedComposedType {
  trivial_shared_ptr<SimpleType> ptr{};

  constexpr NestedComposedType(int v = 0) : ptr(new SimpleType(v)) {}

  constexpr int get() const { return ptr->get(); }
};

suite trivial_shared_ptr_suite = [] {
  "default_construct"_test = [] {
    constexpr auto test = [] {
      trivial_shared_ptr<int> ptr(new int{});
      return ptr.get() != nullptr && *(ptr.get()) == 0;
    };
    static_assert(test());
    expect(test());
  };

  "simple_type_allocation"_test = [] {
    constexpr auto test = [] {
      trivial_shared_ptr<SimpleType> ptr{new SimpleType(100)};
      return ptr.get()->get() == 100;
    };
    static_assert(test());
    expect(test());
  };

  "composed_type_allocation"_test = [] {
    constexpr auto test = [] {
      trivial_shared_ptr<ComposedType> ptr{new ComposedType(200)};
      return ptr.get()->get() == 200;
    };
    static_assert(test());
    expect(test());
  };

  "nested_composed_type_allocation"_test = [] {
    constexpr auto test = [] {
      trivial_shared_ptr<NestedComposedType> ptr{new NestedComposedType(300)};
      return ptr.get()->get() == 300;
    };
    static_assert(test());
    expect(test());
  };

  "copy_construct"_test = [] {
    constexpr auto test = [] {
      trivial_shared_ptr<SimpleType> a{new SimpleType(7)};
      trivial_shared_ptr<SimpleType> b{a};
      return a.get() == b.get() && b.get()->get() == 7;
    };
    static_assert(test());
    expect(test());
  };
};

int main(int, char **) { return 0; }
