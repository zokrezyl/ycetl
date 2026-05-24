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

| Sequence            | Associative (ordered)    | Associative (hashed)              | View / adapter      |
| ------------------- | ------------------------ | --------------------------------- | ------------------- |
| `dynamic_array<T>`  | `set<K>`                 | `unordered_set<K>`                | `span<T>`           |
| `vector<T>`         | `map<K,V>`               | `unordered_map<K,V>`              | `stack<T>`          |
| `array<T,N>`        | `multiset<K>`            | `unordered_multiset<K>`           | `queue<T>`          |
| `list<T>`           | `multimap<K,V>`          | `unordered_multimap<K,V>`         | `priority_queue<T>` |
| `forward_list<T>`   |                          |                                   |                     |
| `deque<T>`          |                          |                                   |                     |
| `basic_string<C>`   |                          |                                   |                     |
| `bitset<N>`         |                          |                                   |                     |
| `hive<T>`           |                          |                                   |                     |

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

## Why not just the C++26 constexpr extensions?

C++26 ships a family of constexpr changes aimed at the same problem
ycetl solves — most notably *non-transient (a.k.a. "less transient" or
"promoted") constexpr allocations* (P3032 and friends), and the
`void* → T*` cast in constant evaluation (P2738). On paper they let you
write `constexpr std::vector<int> v = {...};` and have the allocation
survive into runtime. In practice the design has problems ycetl
sidesteps:

- **Compiler-magic-dependent lifetime.** The standard's promotion rules
  ask the compiler to *prove* an allocation outlives the constant
  evaluation and never mutates afterwards. When it can't prove that,
  you get an opaque "not a constant expression" diagnostic and no
  recourse. ycetl's compile-time → runtime hand-off is a literal copy
  into a result struct the user controls — no proof obligation, no
  silent failure mode.
- **Result layout you don't see.** The C++26 path takes whatever
  internal layout `std::vector` / `std::map` / etc. happened to have
  and embeds *that*. Pointer-laden, padded, with allocator
  bookkeeping. ycetl's result memory is a plain struct of
  `std::array`s and scalars that you write — exact layout, predictable
  size, trivially inspectable in a hex dump, friendly to the
  optimiser.
- **`void*` is back.** P2738 reopens `void* → T*` *in constant
  evaluation*. ycetl was built around the premise that you don't need
  it — a per-type tuple backbone keeps the entire pipeline typed.
  Bringing `void*` back recreates exactly the runtime traps (wrong
  cast, wrong size, wrong alignment) that constexpr was supposed to
  catch.
- **Requires C++26 and a bleeding-edge toolchain.** Not in any release
  of GCC, Clang, or MSVC at the time of writing. ycetl works on
  C++20-clean GCC 12+ / Clang 16+ today.
- **One-way street to runtime.** The C++26 design solves
  "compile-time data → runtime constant", and stops there. ycetl's
  containers are *also* useful at runtime as per-job typed allocators
  — same code, same shape, same lifetime story. No second
  implementation.
- **No discipline on the type universe.** ycetl makes you enumerate
  the type set up front, which feels like a tax but is actually a
  feature: it documents what the computation touches, lets the
  serialiser do its job, and gives `relevant_types` something to
  introspect. The C++26 approach silently traces whatever your code
  happens to allocate, which is fine until it isn't.
- **No nested-tree story.** The promoted-allocation mechanism is
  geared at flat containers (`std::vector<int>`). The moment you want
  a tree of structs containing other containers, you're back to
  hand-rolling — at which point the result-struct + `std::array`
  pattern ycetl uses is exactly what you'd reach for anyway, except
  ycetl already wrote the working-memory side for you.

The trade is honest: ycetl asks for a typed type-set up front and a
hand-written result struct, and in exchange gives you a transparent,
debuggable, runtime-equivalent pipeline that works on shipping
compilers. C++26's promoted constexpr allocations ask you to trust the
compiler's lifetime analysis and accept whatever layout the standard
containers happen to have.

See [doc/overview.md](doc/overview.md) for the full rationale and
the constexpr restrictions that drove this design.

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
