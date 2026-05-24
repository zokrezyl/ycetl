# ycetl — a constexpr-first template library

Header-only C++20 library providing STL-shaped containers, smart pointers, and
a multi-type memory machinery that all work **both at compile time and at
runtime**. Designed for the *yce machine*: build complex data structures
during constant evaluation, then carry the result into runtime as a baked-in
constexpr value.

For the design rationale — why the standard library doesn't fit constexpr,
how multitype memory dodges `void*`, how compile-time data ends up in `.rodata`
— see **[doc/overview.md](doc/overview.md)**.

## Features

### Containers (built on the `dynamic_array` foundation)

| Sequence            | Associative (ordered)    | Associative (hashed)              | View / adapter    |
| ------------------- | ------------------------ | --------------------------------- | ----------------- |
| `dynamic_array<T>`  | `set<K>`                 | `unordered_set<K>`                | `span<T>`         |
| `vector<T>` *(WIP)* | `map<K,V>`               | `unordered_map<K,V>`              | `stack<T>`        |
| `array<T,N>`        | `multiset<K>`            | `unordered_multiset<K>`           | `priority_queue<T>` |
| `bitset<N>`         | `multimap<K,V>`          | `unordered_multimap<K,V>`         |                   |

Every container works under `static_assert` inside a constexpr lambda **and**
at runtime — see the `test_*.cpp` files for the canonical patterns.

### Smart pointers
- `unique_ptr<T>` — move-only owner
- `shared_ptr<T>` / `weak_ptr<T>` — two-counter control block
- `trivial_shared_ptr<T>` — minimal single-counter variant

### Memory machinery (the *yce machine*)
- `type_set<Ts…>` — compile-time, dedup'd type list
- `multitype_handler<H, Ts…>` — `tuple<H<T>…>`-backed dispatch (no `void*`)
- `multitype_memory<Backend, Ts…>` — typed `allocate<T>` / `deallocate<T>`
- `typed_dynamic_memory<T>` — heap-backed per-T backend
- `typed_static_memory<T>` — fixed-capacity per-T backend
- `hash<T>` — constexpr-safe replacement for `std::hash<T>`

### Examples
- `examples/yce/compile_time_primes.cpp` — sieve runs in working memory at
  constexpr time, result lands in a `std::array` baked into the binary.
- `examples/yce/compile_time_nested_tree.cpp` — three-level tree
  (bucket → number → factor) built at constexpr time, ~6 KB of `.rodata`,
  with `static_assert`s that walk every level.
- `examples/wgpu_glue/` — libclang walks `dawn/webgpu.h`, emits both a Python
  `ctypes` module and a `constexpr` C++ tree of the same API. Off by default;
  configure with `-DYCETL_BUILD_WGPU_GLUE=ON`.

## Build

```sh
make                   # configure (if needed) + build, into build-linux/
make test              # ctest
make help              # all targets / overridable variables
```

Or directly via CMake:

```sh
cmake -S . -B build-linux -G Ninja
cmake --build build-linux
ctest --test-dir build-linux
```

Options:

| Option                       | Default | Effect                                              |
| ---------------------------- | ------- | --------------------------------------------------- |
| `YCETL_BUILD_TESTS`          | `ON`    | build `unit_test/`                                  |
| `YCETL_BUILD_EXAMPLES`       | `ON`    | build `examples/yce/`                               |
| `YCETL_BUILD_WGPU_GLUE`      | `OFF`   | build the libclang→ctypes example (needs libclang)  |

## Quick taste

```cpp
#include <ycetl/map.hpp>
#include <ycetl/memory.hpp>
#include <array>

constexpr auto build_table() {
    ycetl::default_memory<std::pair<int, int>> mem;
    ycetl::map<int, int> m(mem);
    for (int i = 1; i <= 5; ++i) m.insert({i, i * i});

    std::array<int, 5> out{};
    int idx = 0;
    for (auto &p : m) out[idx++] = p.second;
    return out;
}

constexpr auto squares = build_table();   // {1, 4, 9, 16, 25} — in .rodata
static_assert(squares[4] == 25);
```

The `ycetl::map` is dynamically grown at constexpr time using
`typed_dynamic_memory`; the result is hand-rolled into a plain
`std::array` so it survives as a runtime constant. See
[doc/overview.md](doc/overview.md) for the full pattern.

## Layout

```
include/ycetl/      header-only library
unit_test/          container, type-system, concept tests (ctest-driven)
examples/yce/       compile-time-to-runtime demos
examples/wgpu_glue/ libclang → ycetl tree → Python ctypes generator
doc/                design + rationale documents
```
