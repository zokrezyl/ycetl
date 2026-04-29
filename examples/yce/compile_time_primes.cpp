// End-to-end demo of the yce machine:
//
//   1. At compile time, allocate working buffers from a typed multitype_memory
//      (one bool buffer + one std::pair<int,int> buffer, dispatched by type).
//   2. Run a sieve of Eratosthenes; collect each prime and its square.
//   3. Serialize the working result into a fixed-size std::array embedded in
//      a struct -- the "result memory" the runtime can read.
//   4. At runtime, print the table.
//
// Every allocation lives in its own typed tuple slot inside multitype_memory,
// returns a typed pointer, and is destroyed before the constexpr context
// exits. No void* casts, no type erasure.

#include <array>
#include <cstddef>
#include <iostream>
#include <utility>

#include <ycetl/memory.hpp>

namespace {

template <std::size_t Capacity>
struct primes_result {
    std::array<std::pair<int, int>, Capacity> records{}; // (prime, prime^2)
    std::size_t count{0};
};

template <int N, std::size_t Capacity>
constexpr primes_result<Capacity> compute_primes_up_to() {
    primes_result<Capacity> out{};

    // Working memory: typed buffers for bool (sieve) and std::pair<int,int>
    // (records). multitype_memory aggregates one typed_dynamic_memory<T>
    // per T into a tuple, so allocate<T>() goes straight to the right slot.
    ycetl::default_memory<bool, std::pair<int, int>> mem;

    // Sieve buffer from the bool slot.
    auto sieve = mem.template allocate<bool>(N + 1);
    for (int i = 0; i <= N; ++i)
        sieve.get()[i] = true;
    sieve.get()[0] = false;
    sieve.get()[1] = false;
    for (int i = 2; i * i <= N; ++i) {
        if (sieve.get()[i]) {
            for (int j = i * i; j <= N; j += i)
                sieve.get()[j] = false;
        }
    }

    // Records buffer from the std::pair<int,int> slot -- a "complex" type.
    auto records = mem.template allocate<std::pair<int, int>>(Capacity);

    std::size_t idx = 0;
    for (int i = 2; i <= N && idx < Capacity; ++i) {
        if (sieve.get()[i]) {
            records.get()[idx] = std::pair<int, int>{i, i * i};
            ++idx;
        }
    }

    // Hand off into result memory (plain std::array baked into the struct).
    out.count = idx;
    for (std::size_t i = 0; i < idx; ++i)
        out.records[i] = records.get()[i];

    // Tear down working memory before the constexpr context ends.
    mem.template deallocate<std::pair<int, int>>(records, Capacity);
    mem.template deallocate<bool>(sieve, N + 1);

    return out;
}

} // namespace

int main() {
    constexpr auto baked = compute_primes_up_to<100, 32>();

    // Proof the table is fully baked at compile time.
    static_assert(baked.count == 25, "there are 25 primes <= 100");
    static_assert(baked.records[0].first == 2);
    static_assert(baked.records[0].second == 4);
    static_assert(baked.records[24].first == 97);
    static_assert(baked.records[24].second == 97 * 97);

    std::cout << "compile-time-computed primes <= 100 (" << baked.count
              << "):\n";
    for (std::size_t i = 0; i < baked.count; ++i) {
        std::cout << "  " << baked.records[i].first
                  << "  (squared = " << baked.records[i].second << ")\n";
    }
    return 0;
}
