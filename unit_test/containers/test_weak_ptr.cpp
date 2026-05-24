#include <boost/ut.hpp>
#include <utility>
#include <ycetl/shared_ptr.hpp>
#include <ycetl/weak_ptr.hpp>

using namespace boost::ut;
using namespace ycetl;

suite weak_ptr_suite = [] {
    "default_construct_is_expired"_test = [] {
        constexpr auto t = [] {
            weak_ptr<int> w;
            return w.expired() && w.use_count() == 0 && !w.lock();
        };
        static_assert(t());
        expect(t());
    };

    "lock_while_strong_alive"_test = [] {
        constexpr auto t = [] {
            shared_ptr<int> s(new int(42));
            weak_ptr<int>   w(s);

            bool ok = !w.expired() && w.use_count() == 1;
            // lock() bumps strong count for the duration of the
            // returned shared_ptr.
            {
                auto locked = w.lock();
                ok = ok && bool(locked) && *locked == 42
                  && w.use_count() == 2 && s.use_count() == 2;
            }
            // Back to 1 after locked goes out of scope.
            return ok && s.use_count() == 1;
        };
        static_assert(t());
        expect(t());
    };

    "expired_after_strong_dies"_test = [] {
        constexpr auto t = [] {
            weak_ptr<int> w;
            {
                shared_ptr<int> s(new int(7));
                w = s;
                if (w.expired() || *w.lock() != 7) return false;
            }
            // The shared_ptr is gone — the managed int has been
            // destroyed, but the control block lives on because w
            // still references it. lock() must return empty.
            return w.expired() && w.use_count() == 0 && !w.lock();
        };
        static_assert(t());
        expect(t());
    };

    "copy_weak_increments_weak_count"_test = [] {
        // We can't observe weak_count_ directly, but if a copy bumped
        // it correctly the control block should outlive the strong
        // and the second weak should still expire cleanly.
        constexpr auto t = [] {
            weak_ptr<int> w1;
            {
                shared_ptr<int> s(new int(11));
                w1 = s;
            }
            weak_ptr<int> w2(w1);
            return w1.expired() && w2.expired();
        };
        static_assert(t());
        expect(t());
    };

    "reset_releases_observer"_test = [] {
        constexpr auto t = [] {
            shared_ptr<int> s(new int(3));
            weak_ptr<int>   w(s);
            w.reset();
            return w.expired() && !w.lock() && s.use_count() == 1;
        };
        static_assert(t());
        expect(t());
    };

    "shared_ptr_alone_still_works"_test = [] {
        // The shared_ptr changes (extra weak_count_) shouldn't have
        // disturbed the no-weak-ptr fast path.
        constexpr auto t = [] {
            shared_ptr<int> a(new int(100));
            bool ok = a.use_count() == 1 && *a == 100;
            {
                shared_ptr<int> b = a;
                ok = ok && a.use_count() == 2 && b.use_count() == 2;
            }
            return ok && a.use_count() == 1;
        };
        static_assert(t());
        expect(t());
    };
};

int main() {}
