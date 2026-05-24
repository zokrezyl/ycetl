# The yce machine: A Comprehensive Guide

**Document Date:** 2025-07-10 09:19:11  
**Author:** zokrezyl

## Introduction

The yce machine represents a powerful abstraction for compile-time computation, leveraging C++'s constexpr capabilities to perform complex operations during compilation rather than at runtime. This document explores the architecture, challenges, and solutions developed for this compile-time computation framework.

## Core Concepts

### Computation Model

At its heart, the yce machine operates on the concept of computations - self-contained units of work that can be evaluated at compile-time. Unlike runtime computation which executes when a program runs, constexpr computation happens during compilation, with results embedded directly in the final binary.

```cpp
// Example of a basic computation
struct FibonacciComputation {
    constexpr static std::uint64_t compute(std::uint64_t n) {
        if (n <= 1) return n;
        return compute(n-1) + compute(n-2);
    }
    
    constexpr static std::uint64_t result = compute(45);
};

// Wrapper to access computation results
template<typename Computation>
struct ComputationWrapper {
    constexpr static auto value = Computation::result;
};

// Usage
constexpr ComputationWrapper<FibonacciComputation> fibonacci_results;
```

### Memory Model

The yce machine operates with two distinct memory concepts:

1. **Working Memory**: A dynamic allocation system that operates during constexpr evaluation. Unlike traditional constexpr which forbids dynamic memory, the yce machine builds working memory out of **one typed buffer per type**, aggregated by `multitype_memory`, so that every allocation stays typed end-to-end and never needs to round-trip through `void*`. This shape is forced by a hard constexpr constraint: while casting a `T*` to `void*` is allowed, casting `void*` back to `T*` is **forbidden** in constant evaluation, so the runtime trick of "one byte buffer + `static_cast` on the way out" cannot work at compile time. The consequence — and the only real cost of the design — is that **every type the computation will allocate must be enumerated up front** as part of the type set. In return, complex data structures can be built and manipulated during compilation, with all memory automatically cleaned up as required by the constexpr specification.

2. **Result Memory**: The final output storage of a computation. Result memory is what gets embedded into the program binary and may be massive in size (potentially hundreds of megabytes), creating challenges for both compilation and optimization.

The high-level model for a computation typically follows this pattern:

```cpp
struct Computation {
    ComputationResult result;  // Result memory - persists in binary

    constexpr Computation() {
        // Working memory: one typed backend per T, aggregated as
        // tuple<typed_dynamic_memory<Ts>...> inside multitype_memory.
        // All types that will be allocated must appear in the set.
        default_memory<Node, Edge, std::int64_t /* …everything we'll allocate… */> memory;

        // Compute. Each `memory.allocate<T>(n)` lands in the per-T tuple slot
        // and returns a typed `T*` — no void* in sight.
        auto intermediate_result = compute(memory);

        // Serialize the working representation into the (compact) result memory.
        serialize(intermediate_result, result);

        // Working memory is automatically cleaned up at end of constexpr context.
    }
};
```

## Type Management

### type_set

The `type_set` serves as a compile-time container for heterogeneous types, enabling the machine to work with collections of different types in a uniform manner. It carries set semantics — duplicates collapse — and supports concat/init/back/tail operations (see `include/ycetl/type_system.hpp`). This is essential for generic programming within the constexpr context, and in particular for enumerating the working-memory type universe up front.

```cpp
template <typename... Ts>
struct type_set {};

// Example usage
using numeric_types = type_set<int, float, double, long>;
```

### multitype_handler

The `multitype_handler` is the structural backbone of `multitype_memory`. It owns a tuple of per-type handler instances and provides the lookup that turns a compile-time `T` into the right tuple slot — which is how the machine maintains type safety across heterogeneous data without ever resorting to `void*`-based erasure (forbidden in constexpr, as established above).

```cpp
template <template <typename> class HandlerImpl, typename TypeSet>
class multitype_handler_impl;

template <template <typename> class HandlerImpl, typename... Ts>
class multitype_handler_impl<HandlerImpl, type_set<Ts...>> {
    std::tuple<HandlerImpl<Ts>...> _handlers;   // one HandlerImpl per T

public:
    template <typename T>
    constexpr HandlerImpl<T>& get_handler() {
        return std::get<HandlerImpl<T>>(_handlers);
    }

    template <typename T>
    constexpr const HandlerImpl<T>& get_handler() const {
        return std::get<HandlerImpl<T>>(_handlers);
    }
};

// Public form that flattens a parameter pack into a deduplicated type_set:
template <template <typename> class HandlerImpl, typename... RawTypes>
class multitype_handler
    : public multitype_handler_impl<HandlerImpl, flat_type_set_t<RawTypes...>> {};
```

Because every dispatch is `std::get<HandlerImpl<T>>(_handlers)` — a compile-time index into a `std::tuple` — the resulting `T*` is fully typed without any cast, which is what makes the whole pipeline constexpr-legal.

### multitype_memory

`multitype_memory` is the cornerstone of the yce machine's memory management system. Built on top of `multitype_handler`, it provides a type-safe container for dynamically allocated heterogeneous data during constexpr computation.

Restating the core insight: **all possible types must be known before the computation begins**. This is not a stylistic preference — it follows directly from the language rules:

1. Constexpr forbids recovery of type information from type-erased pointers (`void*` → `T*` via `static_cast` is rejected during constant evaluation).
2. Without the ability to recover types, memory cannot be safely managed at the end of computation.
3. The eventual serialization step into result memory needs the type identities anyway.

So `multitype_memory` parameterizes a per-type backend (`MemoryBackend<T>`) over the enumerated type set, and dispatches each typed `allocate<T>` / `deallocate<T>` to the right tuple slot:

```cpp
template <template <typename> class MemoryBackend, typename... RawType>
class multitype_memory : public multitype_handler<MemoryBackend, RawType...> {
public:
    // The pointer type the per-T backend hands out (typically T*).
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
template <typename... RawType>
using dynamic_memory = multitype_memory<typed_dynamic_memory, RawType...>;

template <typename... RawType>
using default_memory = dynamic_memory<RawType...>;

template <typename... RawType>
using static_memory  = multitype_memory<typed_static_memory, RawType...>;
```

The per-type backend (e.g. `typed_dynamic_memory<T>`) holds its own typed allocator and bookkeeping for that single type — see `include/ycetl/typed_dynamic_memory.hpp`. There are two common backends today:

- `typed_dynamic_memory<T>` — heap-style, suitable for working memory whose size is computed during constexpr evaluation.
- `typed_static_memory<T>` — fixed-capacity, useful for the result side / known-bound buffers.

Because every allocation is dispatched as `get_handler<T>().allocate(n)` (a compile-time tuple index into `tuple<MemoryBackend<Ts>...>`) and returns the per-T backend's native pointer (typically `T*`), nothing in the chain ever requires a `void*→T*` cast. The "all types known up front" rule is what buys us this property.

During the actual computation, working memory managed by `multitype_memory` allows complex data structures to be built and manipulated, while the explicit type tracking lets the data be serialized into the compact, type-specific representation in the result memory.

## Serialization

A core concept in the yce machine is serialization, which bridges the gap between the dynamic working memory and the static result memory.

### Purpose of Serialization

1. **Type Erasure**: Converting heterogeneous types into a uniform representation.
2. **Memory Compaction**: Eliminating overhead from the working memory's dynamic structure.
3. **Permanence**: Transferring from temporary working memory to permanent result memory.

```cpp
// Example of serialization
template <typename Result>
constexpr void serialize(const WorkingData& data, Result& result) {
    // Convert complex, possibly nested data structures
    // into a compact, flattened representation
    for (size_t i = 0; i < data.size(); i++) {
        // Extract and compact each element
        result.data[i] = extract_value(data, i);
    }
    
    // Additional metadata may be serialized as well
    result.metadata = compute_metadata(data);
}
```

Serialization enables the yce machine to work with rich, complex data structures during computation, but still produce optimized, memory-efficient final results that can be embedded in the binary.

## Challenges and Solutions

### Memory Limitations

**Challenge:** Compiler implementations limit the depth of constexpr recursion and the size of constexpr objects.

**Solution:** The dynamic allocation system in working memory allows for complex data structures that would otherwise exceed the constexpr limitations, with careful serialization to result memory.

### Type Heterogeneity

**Challenge:** Handling different types uniformly in constexpr contexts is difficult due to the strict typing of C++.

**Solution:** The combination of `type_set`, `multitype_handler`, and `multitype_memory` provides a comprehensive framework for type-agnostic programming at compile-time. Crucially, "type-agnostic" here means uniform *handling* of many types via per-type tuple slots, **not** type erasure — `void*` is never used.

### Compilation Time

**Challenge:** Large constexpr computations can lead to extremely long compilation times.

**Solution:** We implemented memoization techniques and compilation caching to avoid redundant computations. Additionally, the architecture allows for incremental compilation of different computation units.

### Size Constraints

**Challenge:** Some constexpr computations generate extremely large result sets (hundreds of MB).

**Solution:** We developed a custom memory allocation scheme that efficiently packs data and leverages template specialization to handle different size requirements optimally.

```cpp
// Example of size-determined computation
struct SizeComputation {
    constexpr static std::uint64_t compute() {
        // Logic to determine size based on compilation parameters
        return /* large size value */;
    }
    
    constexpr static std::uint64_t result = compute();
};

// Use the computed size in another computation
constexpr ComputationWrapper<SizeComputation> size_computation;
constexpr HugeComputation<size_computation.value> huge_data;
```

## Practical Applications

The yce machine enables several powerful use cases:

1. **Cryptographic Tables:** Pre-computing lookup tables for cryptographic algorithms.
2. **Game Assets:** Embedding complex game data directly in the binary.
3. **Mathematical Constants:** Computing high-precision mathematical constants.
4. **Code Generation:** Generating optimized code paths based on compile-time analysis.

## Future Directions

As C++ standards evolve, we anticipate expanding the yce machine's capabilities:

1. Leveraging C++20 concepts for more expressive constraints.
2. Exploring integration with consteval for guaranteed compile-time evaluation.
3. Developing distributed compilation techniques for extremely large computations.

## Conclusion

The yce machine represents a significant advancement in compile-time computation, pushing the boundaries of what's possible with C++ templates and constexpr. Through its innovative approach to working memory, type management, and serialization, it enables developers to perform complex computations at compile-time that were previously impossible. The result is more efficient binaries with rich, pre-computed data structures, offering both performance and expressiveness benefits over traditional runtime approaches.
