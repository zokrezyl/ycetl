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
      trivial_shared_ptr<int> ptr;
      return ptr.get() == nullptr;
    };
    expect(test());
  };

  "construct_from_pointer"_test = [] {
    constexpr auto test = [] {
      int value = 42;
      trivial_shared_ptr<int> ptr(&value);
      return ptr.get() == &value && *ptr == 42_i;
    };
    expect(test());
  };

  "simple_type_allocation"_test = [] {
    constexpr auto test = [] {
      trivial_shared_ptr<SimpleType> ptr(new SimpleType(100));
      return ptr.get()->get() == 100_i;
    };
    expect(test());
  };

  "composed_type_allocation"_test = [] {
    constexpr auto test = [] {
      trivial_shared_ptr<ComposedType> ptr(new ComposedType(200));
      return ptr.get()->get() == 200_i;
    };
    expect(test());
  };

  "copy_construct"_test = [] {
    constexpr auto test = [] {
      trivial_shared_ptr<SimpleType> a(new SimpleType(7));
      trivial_shared_ptr<SimpleType> b(a);
      return a.get() == b.get() && b.get()->get() == 7_i;
    };
    expect(test());
  };
};

int main(int argc, char **argv) { return 0; }
