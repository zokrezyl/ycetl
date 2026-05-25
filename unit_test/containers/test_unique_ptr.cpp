// SPDX-License-Identifier: MIT

#include <boost/ut.hpp>
#include <utility>
#include <ycetl/unique_ptr.hpp>

using namespace boost::ut;
using namespace ycetl;

namespace {
// Plain struct so the constexpr new / delete is trivially constexpr-clean.
struct value {
  int x;
  constexpr value() : x(0) {}
  constexpr value(int v) : x(v) {}
};
} // namespace

suite unique_ptr_suite = [] {
  "make_unique_and_deref"_test = [] {
    constexpr auto t = [] {
      auto p = make_unique<int>(42);
      return bool(p) && *p == 42;
    };
    static_assert(t());
    expect(t());
  };

  "move_transfers_ownership"_test = [] {
    constexpr auto t = [] {
      auto a = make_unique<int>(7);
      auto b = std::move(a);
      return !a && b && *b == 7;
    };
    static_assert(t());
    expect(t());
  };

  "reset_replaces_object"_test = [] {
    constexpr auto t = [] {
      auto p = make_unique<int>(1);
      p.reset(new int(99));
      return *p == 99;
    };
    static_assert(t());
    expect(t());
  };

  "reset_to_null"_test = [] {
    constexpr auto t = [] {
      auto p = make_unique<int>(123);
      p.reset();
      return !p && p.get() == nullptr;
    };
    static_assert(t());
    expect(t());
  };

  "release_yields_pointer"_test = [] {
    constexpr auto t = [] {
      auto p = make_unique<int>(5);
      int *raw = p.release();
      bool ok = !p && raw && *raw == 5;
      delete raw; // matching delete keeps the constexpr eval balanced
      return ok;
    };
    static_assert(t());
    expect(t());
  };

  "arrow_operator"_test = [] {
    constexpr auto t = [] {
      auto p = make_unique<value>(33);
      return p->x == 33;
    };
    static_assert(t());
    expect(t());
  };

  "swap"_test = [] {
    constexpr auto t = [] {
      auto a = make_unique<int>(1);
      auto b = make_unique<int>(2);
      a.swap(b);
      return *a == 2 && *b == 1;
    };
    static_assert(t());
    expect(t());
  };

  "null_compare"_test = [] {
    constexpr auto t = [] {
      unique_ptr<int> p;
      return p == nullptr && !p;
    };
    static_assert(t());
    expect(t());
  };
};

int main() {}
