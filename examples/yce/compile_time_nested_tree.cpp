// Nested compile-time tree → runtime value.
//
// Builds a three-level tree at constexpr time:
//
//   bucket[B]            — one entry per decade [10·B .. 10·B + 9]
//     └── number[N]      — every n in that decade
//           └── factor[F]— prime factors of n (with multiplicity)
//
// Working memory is a typed multitype_memory<int>; the sieve and the
// per-number factor list both live in the bool/int slots. The result
// gets serialised into a static-sized tree (std::array of std::array of
// std::array of int) baked into a struct — the same compile-time → runtime
// hand-off shown in compile_time_primes.cpp, but with two extra layers of
// nesting to confirm the pattern holds.
//
// At main() time the tree is a plain constexpr value: every level can be
// indexed with static_assert, and the runtime walks it without any
// dynamic allocation.

#include <array>
#include <cstddef>
#include <iostream>

#include <ycetl/memory.hpp>

namespace {

constexpr int kMax            = 99;   // build the tree for [2..99]
constexpr int kBuckets        = 10;   // 10 decades: [10·B..10·B+9]
constexpr int kPerBucket      = 10;   // 10 numbers per decade
constexpr int kMaxFactors     = 7;    // 64 = 2^6, plenty of headroom for [2..99]

struct factor_entry {
    int prime;
    int exponent;
};

struct number_entry {
    int n;
    int factor_count;
    std::array<factor_entry, kMaxFactors> factors;
};

struct bucket_entry {
    int decade_start;
    int populated;
    std::array<number_entry, kPerBucket> numbers;
};

struct factor_tree {
    std::array<bucket_entry, kBuckets> buckets;
};

// Build the sieve once in working memory and reuse it across numbers.
constexpr factor_tree build_tree()
{
    factor_tree out{};

    ycetl::default_memory<bool, int> mem;
    auto sieve = mem.template allocate<bool>(kMax + 1);
    for (int i = 0; i <= kMax; ++i) sieve.get()[i] = true;
    sieve.get()[0] = false;
    sieve.get()[1] = false;
    for (int i = 2; i * i <= kMax; ++i) {
        if (sieve.get()[i]) {
            for (int j = i * i; j <= kMax; j += i) sieve.get()[j] = false;
        }
    }

    // A scratch list of primes <= kMax — sized for the worst case so we
    // don't have to know the prime count up front.
    auto primes = mem.template allocate<int>(kMax + 1);
    int prime_count = 0;
    for (int i = 2; i <= kMax; ++i)
        if (sieve.get()[i]) primes.get()[prime_count++] = i;

    for (int b = 0; b < kBuckets; ++b) {
        bucket_entry &bucket = out.buckets[b];
        bucket.decade_start  = b * 10;
        bucket.populated     = 0;

        for (int k = 0; k < kPerBucket; ++k) {
            int n = b * 10 + k;
            if (n < 2) continue;

            number_entry &ne = bucket.numbers[bucket.populated++];
            ne.n            = n;
            ne.factor_count = 0;

            int rem = n;
            for (int p = 0; p < prime_count && rem > 1; ++p) {
                int prime = primes.get()[p];
                if (prime * prime > rem && rem > 1) {
                    // rem itself is prime — record it and stop.
                    ne.factors[ne.factor_count++] = {rem, 1};
                    rem = 1;
                    break;
                }
                int exp = 0;
                while (rem % prime == 0) { rem /= prime; ++exp; }
                if (exp > 0) ne.factors[ne.factor_count++] = {prime, exp};
            }
        }
    }

    mem.template deallocate<int>(primes, kMax + 1);
    mem.template deallocate<bool>(sieve, kMax + 1);
    return out;
}

// Walk the tree at constexpr time to find the factorisation of n. Used
// by static_asserts to prove arbitrary depths of the tree are reachable
// in a constant expression.
constexpr const number_entry *find_number(const factor_tree &t, int n)
{
    int b = n / 10;
    if (b >= kBuckets) return nullptr;
    const bucket_entry &bucket = t.buckets[b];
    for (int i = 0; i < bucket.populated; ++i) {
        if (bucket.numbers[i].n == n) return &bucket.numbers[i];
    }
    return nullptr;
}

constexpr int recompose(const number_entry &ne)
{
    int v = 1;
    for (int i = 0; i < ne.factor_count; ++i) {
        for (int e = 0; e < ne.factors[i].exponent; ++e) v *= ne.factors[i].prime;
    }
    return v;
}

} // namespace

int main()
{
    // The whole nested tree is a constexpr value embedded in .rodata.
    // `static` matters: subobject pointers into a *local* constexpr
    // variable are not themselves constant expressions, so the
    // static_asserts below would fail to evaluate. Static storage gives
    // the tree a stable address, which is what the standard requires for
    // a pointer into it to be a core constant expression.
    static constexpr auto tree = build_tree();

    // Compile-time round-trip checks across all three levels:
    //   buckets → numbers → factors
    static_assert(tree.buckets.size() == kBuckets);
    static_assert(tree.buckets[1].decade_start == 10);
    static_assert(tree.buckets[1].populated     == kPerBucket);

    // 12 = 2² · 3
    constexpr const number_entry *twelve = find_number(tree, 12);
    static_assert(twelve != nullptr);
    static_assert(twelve->factor_count == 2);
    static_assert(twelve->factors[0].prime == 2 && twelve->factors[0].exponent == 2);
    static_assert(twelve->factors[1].prime == 3 && twelve->factors[1].exponent == 1);

    // 97 is prime
    constexpr const number_entry *p97 = find_number(tree, 97);
    static_assert(p97 != nullptr);
    static_assert(p97->factor_count == 1);
    static_assert(p97->factors[0].prime == 97 && p97->factors[0].exponent == 1);

    // 60 = 2² · 3 · 5 — three distinct primes, exercises a deeper factor list.
    constexpr const number_entry *p60 = find_number(tree, 60);
    static_assert(p60 != nullptr);
    static_assert(p60->factor_count == 3);
    static_assert(p60->factors[0].prime == 2 && p60->factors[0].exponent == 2);
    static_assert(p60->factors[1].prime == 3 && p60->factors[1].exponent == 1);
    static_assert(p60->factors[2].prime == 5 && p60->factors[2].exponent == 1);

    // Every populated entry round-trips: ∏ pᵢ^eᵢ == n. This walks the
    // *entire* tree at compile time — buckets × numbers × factors.
    static_assert([] {
        constexpr auto t = build_tree();
        for (int b = 0; b < kBuckets; ++b) {
            const bucket_entry &bucket = t.buckets[b];
            for (int i = 0; i < bucket.populated; ++i) {
                if (recompose(bucket.numbers[i]) != bucket.numbers[i].n)
                    return false;
            }
        }
        return true;
    }());

    // Runtime walk — same data, same shape, no allocation.
    std::cout << "compile-time-built factor tree for [2.." << kMax << "]:\n";
    for (const auto &bucket : tree.buckets) {
        if (bucket.populated == 0) continue;
        std::cout << "  decade " << bucket.decade_start << ".."
                  << bucket.decade_start + 9 << " ("
                  << bucket.populated << " numbers):\n";
        for (int i = 0; i < bucket.populated; ++i) {
            const number_entry &ne = bucket.numbers[i];
            std::cout << "    " << ne.n << " = ";
            if (ne.factor_count == 0) { std::cout << "?\n"; continue; }
            for (int f = 0; f < ne.factor_count; ++f) {
                if (f) std::cout << " * ";
                std::cout << ne.factors[f].prime;
                if (ne.factors[f].exponent > 1)
                    std::cout << "^" << ne.factors[f].exponent;
            }
            std::cout << "\n";
        }
    }
    std::cout << "tree size in bytes: " << sizeof(tree) << "\n";
    return 0;
}
