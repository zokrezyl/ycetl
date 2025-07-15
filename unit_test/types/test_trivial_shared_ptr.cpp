#include <boost/ut.hpp>
#include <ycetl/trivial_shared_ptr.hpp>

using namespace boost::ut;
using namespace ycetl;

struct SimpleType {
  int *value{};

  constexpr SimpleType(int v) : value(new int(v)) {}
  constexpr ~SimpleType() { delete value; }

  constexpr int get() const { return *value; }
};

struct ComposedType {
  SimpleType *simple{};

  constexpr ComposedType(int v) : simple(new SimpleType(v)) {}
  constexpr ~ComposedType() { delete simple; }

  constexpr int get() const { return simple->get(); }
};

suite trivial_shared_ptr_suite = [] {
  "default_construct"_test = [] {
    constexpr auto test = [] {
      trivial_shared_ptr<int> ptr{};
      return ptr.get() == nullptr;
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

int main(int argc, char **argv) { return 0; }
