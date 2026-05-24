# ycetl & the yce machine — design overview

> **What the README covers:** what's in the library and how to build it.
> **What this document covers:** *why* the library is shaped the way it is —
> the constexpr constraints it works around, the memory model it builds on
> top of them, and how compile-time data ends up as a runtime value.

## Why the standard library doesn't fit constexpr

C++20 added `constexpr new` / `constexpr delete`, which sounds like it
unblocks every container — but in practice almost nothing in the standard
library works under constant evaluation. The roadblocks:

- **No type erasure.** `void*` → `T*` via `static_cast` is rejected during
  constant evaluation. Any allocator that stores type-erased bytes and
  reconstitutes typed pointers on the way out (the model behind
  `std::vector`, `std::any`, polymorphic allocators…) is unusable at compile
  time.
- **Restricted casting.** `reinterpret_cast` is forbidden outright. Most
  generic memory schemes fall over the moment they need an unchecked cast.
- **Strict allocation lifetime.** Memory allocated with `constexpr new` must
  be deallocated within the *same* constant evaluation. You can't allocate
  inside one constexpr function, return the pointer, and let the caller free
  it later — or carry the buffer past the end of the evaluation into
  runtime.
- **Limited stdlib coverage.** Most STL containers and their methods were
  never marked `constexpr`, or rely on runtime-only mechanics (mutex,
  exception-based control flow, type-erased allocators). Trying to use
  `std::unordered_map` in a `static_assert` lambda is an immediate
  compile error.
- **No "freeze and ship".** Even when you can build a structure at compile
  time using `constexpr new`, there's no in-language way to take the
  resulting heap of objects and embed it directly into the binary's
  `.rodata` section for runtime read-only use.
- **Other restrictions.** Constexpr forbids I/O, exception-based control
  flow, certain forms of global mutable state, and caps recursion depth /
  evaluation step count via tunable but finite limits.

These aren't minor inconveniences — they're structural barriers that make
compile-time computation hard to scale beyond toy examples.

## The yce machine, in one paragraph

The yce machine sidesteps the constexpr restrictions with a single insight:
**enumerate every type the computation will allocate up front, and use that
list to build a typed memory backbone with no `void*` anywhere.** Working
memory becomes a tuple of per-type backends (one
`typed_dynamic_memory<T>` slot per `T`); `allocate<T>(n)` lands in the right
slot and returns a typed `T*`. Heterogeneous data is handled by tuple
indexing at compile time, never by erasure. The constexpr rules permit
this because the type identity is preserved through every operation.

Once a computation has produced its working tree, the result is *copied* —
not transferred — into a fixed-shape **result memory** (typically a struct
containing `std::array`s). The working memory tears down at the end of the
constant evaluation as the rules require; the result struct survives as a
`constexpr` value, which the compiler is free to bake into the binary.

## Memory model

Two memory kinds, with very different lifetimes:

| Kind            | Backing                              | Lives in                | Notes                                           |
| --------------- | ------------------------------------ | ----------------------- | ----------------------------------------------- |
| Working memory  | `multitype_memory<Backend, Ts…>`     | constant evaluation     | Tear-down required before the eval ends         |
| Result memory   | plain struct of `std::array`s        | `.rodata` of the binary | Survives as a `constexpr` value                 |

The canonical shape of a yce computation:

```cpp
struct Computation {
    ComputationResult result;   // ← embedded in the binary

    constexpr Computation() {
        // Working memory: one typed backend per T, aggregated as
        // tuple<typed_dynamic_memory<Ts>...> inside multitype_memory.
        // All types that will be allocated must appear in the set.
        ycetl::default_memory<Node, Edge, std::int64_t /*…*/> memory;

        // Compute. Each `memory.allocate<T>(n)` lands in the per-T tuple
        // slot and returns a typed T* — no void* in sight.
        auto intermediate = compute(memory);

        // Serialize the working representation into the (compact)
        // fixed-shape result memory.
        serialize(intermediate, result);

        // Working memory tears down automatically at the end of the
        // constexpr context.
    }
};

constexpr Computation baked;   // result is now a constant
```

`examples/yce/compile_time_primes.cpp` and `compile_time_nested_tree.cpp`
follow exactly this shape end-to-end, including the
`static constexpr auto t = build()` discipline for pointers-into-self
(see below).

### The "all types known up front" rule

This is the only non-obvious constraint the design imposes on a user. It
falls out of the language rules — there is no way to dispatch on an
unknown `T` at compile time without recovering the type from somewhere,
and the standard explicitly forbids the cheapest source (`void* → T*`
casts). Enumerating the type set up front:

- Lets `multitype_memory` instantiate one `MemoryBackend<T>` per `T` at
  compile time.
- Makes every allocation a `std::get<MemoryBackend<T>>(_handlers).allocate(n)`
  — a tuple index, fully typed, constexpr-legal.
- Naturally aligns with the serialization step that follows: the result
  memory has to be sized and typed up front anyway.

The `relevant_types` machinery in `include/ycetl/type_system.hpp`
computes the full type set for a nested container automatically — given
the top-level type, it walks the template arguments transitively and
returns a deduplicated `type_set`.

## Type system

### `type_set<Ts…>`

Compile-time set of types. Supports set operations (concat / init / back /
tail / dedup), is the input shape every other piece of the machine
expects.

```cpp
using numeric_types = ycetl::type_set<int, float, double, long>;
using flat = ycetl::flat_type_set_t<int, float, int, double>;
// flat == type_set<int, float, double>
```

### `multitype_handler<H, Ts…>`

The structural backbone. Owns a `tuple<H<T>…>` and exposes
`get_handler<T>()`. Used by `multitype_memory` (with
`H = MemoryBackend`), but generic — anything that needs a per-T policy
object can use it.

```cpp
template <template <typename> class HandlerImpl, typename... Ts>
class multitype_handler_impl<HandlerImpl, type_set<Ts...>> {
    std::tuple<HandlerImpl<Ts>...> _handlers;
public:
    template <typename T>
    constexpr HandlerImpl<T>& get_handler() {
        return std::get<HandlerImpl<T>>(_handlers);
    }
    // ... const overload ...
};
```

`std::get<HandlerImpl<T>>` is a compile-time tuple index, so the
resulting `HandlerImpl<T>&` is fully typed without any cast — this is
exactly what makes the dispatch constexpr-legal.

### `multitype_memory<Backend, Ts…>`

The user-facing allocator. Parameterised over a per-type backend, gives
you typed `allocate<T>(n)` / `deallocate<T>(p, n)`:

```cpp
template <template <typename> class MemoryBackend, typename... RawType>
class multitype_memory : public multitype_handler<MemoryBackend, RawType...> {
public:
    template <typename T>
    using pointer_type = typename MemoryBackend<T>::pointer;

    template <typename T>
    constexpr auto allocate(std::size_t n) {
        return this->template get_handler<T>().allocate(n);   // returns T*
    }
    template <typename T>
    constexpr void deallocate(pointer_type<T> p, std::size_t n) {
        this->template get_handler<T>().deallocate(p, n);
    }
};

// Common spellings:
template <typename... Ts>
using dynamic_memory = multitype_memory<typed_dynamic_memory, Ts...>;
template <typename... Ts>
using default_memory = dynamic_memory<Ts...>;             // working memory
template <typename... Ts>
using static_memory  = multitype_memory<typed_static_memory, Ts...>;
```

The two stock per-T backends, both in `include/ycetl/`:

- **`typed_dynamic_memory<T>`** — heap-style. Suitable for working
  memory whose size is computed during constexpr evaluation.
- **`typed_static_memory<T>`** — fixed-capacity. Useful for known-bound
  buffers (often the result side).

Because the dispatch is always `get_handler<T>().allocate(n)` and every
backend hands back its native pointer (typically `T*`), the entire
allocation chain stays in typed territory — no `void* → T*` cast
anywhere. The "all types known up front" rule is exactly what buys this.

### Memory downgrading

A `multitype_handler` instantiated over a superset of types can construct
a "downgraded" handler over a subset, sharing the underlying per-type
backends (via `trivial_shared_ptr`). Useful when a nested computation
needs only a subset of the outer type universe — the inner code gets a
narrower handler without copying or re-allocating any backend state.

## Serialization: bridging working to result memory

Serialization is the step that copies the working-memory tree into the
fixed-shape result struct. Its job is three things at once:

1. **Compaction.** The working representation is convenient for
   building (typed pointers, separate per-T buffers); the result wants
   to be a contiguous `std::array`-of-`std::array` shape so the compiler
   can bake it into `.rodata`.
2. **Lifetime hand-off.** Working memory must be deallocated before the
   constant evaluation ends. Serialization is the last opportunity to
   copy data out.
3. **Shape stability.** Result memory has a fixed schema by the time
   the surrounding computation type is instantiated — the size of every
   nested array is part of the type, baked at instantiation time.

A bare-bones serialize loop:

```cpp
template <typename Result>
constexpr void serialize(const WorkingData& data, Result& result) {
    for (size_t i = 0; i < data.size(); ++i)
        result.data[i] = extract_value(data, i);
    result.metadata = compute_metadata(data);
}
```

The real-world version in `examples/yce/compile_time_nested_tree.cpp`
walks a three-level tree (bucket → number → factor) and lands it in a
`factor_tree` struct that comes out to ~6 KB of `.rodata` for `[2..99]`.

## Compile-time data → runtime constant: the canonical pattern

The whole point of the machine is that a `constexpr` computation
produces a value that survives into runtime. The standard shape:

```cpp
constexpr auto baked = build_something();   // file scope, or…

int main() {
    static constexpr auto t = build_something();   // …local with static!
    // …read t at runtime, no allocation
}
```

**Static storage matters for local constexpr values.** A pointer or
reference into a *local* `constexpr` variable isn't itself a constant
expression — the variable has no fixed address. Marking the local
`static constexpr` gives it a stable storage location, which is what
the standard requires for `find(t, k)` to return a pointer that
participates in a later `static_assert`. The nested-tree example calls
this out explicitly at its `main()` boundary.

## What survives, what doesn't

Concrete rules for what comes out of a constexpr context:

- **A plain struct of `std::array`s, `pair`s, scalars** — yes. This is
  the result-memory shape the machine targets.
- **A pointer to a sub-object of a *static* constexpr value** — yes.
- **A pointer to a sub-object of a *local* constexpr value** — no
  (subobject self-reference; the variable has no stable address).
- **A heap object allocated with `constexpr new` and not freed before
  the eval ends** — no, the standard rejects the program.
- **A container whose `_data` pointer points into its own
  `_memory` member** — no (subobject self-reference). Use offsets
  instead of pointers. The `static_container` shape in
  `unit_test/concepts/test_simple.cpp` demonstrates the offset-based
  workaround.

## Container layer

All ycetl containers are built on `dynamic_array<T, TypedMemory>`. The
defaults make `dynamic_array<int>` behave like a `std::vector<int>`
that's also constexpr-clean. Swap `TypedMemory` for
`typed_static_memory<int>` and you get a fixed-capacity variant for
result-memory use.

What's currently implemented:

| Category            | Containers                                                       |
| ------------------- | ---------------------------------------------------------------- |
| Sequence            | `dynamic_array`, `array`, `bitset`                               |
| View / adapter      | `span`, `stack`, `priority_queue`                                |
| Ordered associative | `set`, `map`, `multiset`, `multimap`                             |
| Hashed associative  | `unordered_set`, `unordered_map`, `unordered_multiset`, `unordered_multimap` |
| Smart pointers      | `unique_ptr`, `shared_ptr`, `weak_ptr`, `trivial_shared_ptr`     |

Hash containers use `ycetl::hash<T>` by default (not `std::hash<T>` —
libstdc++ leaves `std::hash<int>::operator()` non-constexpr, which
would block any compile-time use). `ycetl::hash<T>` provides Wang's
mix64 for integral types + pointers and FNV-1a for `string_view`.

`vector`, `list`, and `basic_string` headers exist but are pinned to
an earlier `container::container<>` glue that isn't yet wired up; their
tests are gated off in CMake. Don't use them as-is.

## Runtime use

ycetl is **not limited to constexpr**. The same containers work at
runtime, with the same shape:

- Per-job memory allocators — declare a `default_memory<Ts…>` for the
  job's type universe, build up structures, let everything tear down
  when the job finishes. No locks, no per-allocation `new`/`delete`
  overhead.
- The library is intentionally **single-threaded**. None of the
  smart pointers or containers use atomic counters; the design point
  is "one thread / one constexpr evaluation owns the structure".

## Practical applications

- **Pre-computed lookup tables.** Cryptographic constants, math
  tables, codecs — compute once at build time, ship as `.rodata`.
- **AST → runtime code-gen.** Walk an external description (a header
  file, a schema, a grammar) at build time, produce a constexpr tree
  the runtime can introspect. `examples/wgpu_glue/` is the worked
  example.
- **Compile-time-verified runtime structures.** Embed graphs, state
  machines, parser tables into the binary with `static_assert`s
  enforcing the structural invariants at the same time.

## Pointers to source

| Concept                   | Lives in                                |
| ------------------------- | --------------------------------------- |
| `type_set`, `flat_type_set_t`, relevant types | `include/ycetl/type_system.hpp` |
| `multitype_handler`       | `include/ycetl/multitype_handler.hpp`   |
| `multitype_memory` + spellings | `include/ycetl/multitype_memory.hpp`, `include/ycetl/memory.hpp` |
| Per-T backends            | `include/ycetl/typed_dynamic_memory.hpp`, `include/ycetl/typed_static_memory.hpp` |
| Containers                | `include/ycetl/<name>.hpp`              |
| End-to-end demos          | `examples/yce/compile_time_*.cpp`       |
| Build-time codegen demo   | `examples/wgpu_glue/`                   |
