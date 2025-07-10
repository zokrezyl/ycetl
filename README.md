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

1. **Working Memory**: A dynamic allocation system that operates during constexpr evaluation. Unlike traditional constexpr which forbids dynamic memory, the yce machine implements a specialized allocator that leverages `multitype_storage` to track allocations across the entire call chain. This allows for complex data structures to be built and manipulated during compilation, with all memory automatically cleaned up as required by the constexpr specification.

2. **Result Memory**: The final output storage of a computation. Result memory is what gets embedded into the program binary and may be massive in size (potentially hundreds of megabytes), creating challenges for both compilation and optimization.

The high-level model for a computation typically follows this pattern:

```cpp
struct Computation {
    ComputationResult result;  // Result memory - persists in binary
     
    constexpr Computation() {
        // Working memory pattern
        auto type_list = calculate_typelist();
        auto storage = multitype_storage<type_list>();
        auto allocator = Allocator(storage);

        // Perform computation with working memory
        auto intermediate_result = compute(allocator);
        
        // Serialize working memory into result memory
        serialize(intermediate_result, result);
        
        // Working memory is automatically cleaned up
    }
};
```

## Type Management

### type_list

The `type_list` serves as a compile-time container for heterogeneous types, enabling the machine to work with collections of different types in a uniform manner. This is essential for generic programming within the constexpr context.

```cpp
template <typename... Ts>
struct type_list {};

// Example usage
using numeric_types = type_list<int, float, double, long>;
```

### multitype_handler

The `multitype_handler` exists primarily to support the operations of `multitype_storage`. It provides mechanisms to operate uniformly across different types at compile-time, enabling type-safe operations within the constexpr context where traditional type erasure techniques are restricted.

A crucial constraint in constexpr computation is that while casting to `void*` is theoretically possible, casting back from `void*` to a typed pointer (`static_cast<T*>`) is forbidden. This means standard type-erasure techniques that work at runtime fail in constexpr contexts.

```cpp
template <template <typename> class Operation, typename TypeList>
struct multitype_handler;

template <template <typename> class Operation, typename... Ts>
struct multitype_handler<Operation, type_list<Ts...>> {
    // Apply operation to each type in the type list
    using results = type_list<typename Operation<Ts>::type...>;
    
    // Execute an operation on the appropriate type based on a type index
    template <typename Func>
    constexpr static void dispatch(size_t type_index, Func&& func) {
        // Array of function pointers, one for each type
        constexpr void (*dispatchers[])(Func&&) = {
            [](Func&& f) { f.template operator()<Ts>(); }...
        };
        
        // Call the appropriate function based on type_index
        dispatchers[type_index](std::forward<Func>(func));
    }
    
    // Other utilities for handling multiple types
    // e.g., type comparison, registration, etc.
};
```

The `multitype_handler` enables the yce machine to maintain type safety throughout the computation while still allowing for generic, type-agnostic algorithms to be implemented.

### multitype_storage

The `multitype_storage` component is the cornerstone of the yce machine's memory management system. It provides a type-safe container for dynamically allocated heterogeneous data during constexpr computation.

The critical insight behind `multitype_storage` is that **all possible types must be known before the computation begins**. This is a fundamental requirement because:

1. Constexpr forbids recovery of type information from type-erased pointers (e.g., `static_cast` from `void*`)
2. Without the ability to recover types, we cannot manage the memory correctly at the end of computation
3. We need a mechanism to serialize the final data structure into the result memory

```cpp
template <typename TypeList>
class multitype_storage;

template <typename... Ts>
class multitype_storage<type_list<Ts...>> {
    // Storage for each type in the list
    std::tuple<std::vector<Ts>...> type_storage;
    
    // Type ID mapping to help with lookup
    template <typename T>
    constexpr static size_t type_id = /* implementation */;

public:
    // Allocate an object of type T
    template <typename T>
    constexpr T* allocate() {
        static_assert((std::is_same_v<T, Ts> || ...), 
            "Type T must be in the TypeList");
            
        auto& storage = std::get<std::vector<T>>(type_storage);
        storage.emplace_back();
        return &storage.back();
    }
    
    // Allocate and initialize
    template <typename T, typename... Args>
    constexpr T* construct(Args&&... args) {
        auto& storage = std::get<std::vector<T>>(type_storage);
        storage.emplace_back(std::forward<Args>(args)...);
        return &storage.back();
    }
    
    // Access objects by type and index
    template <typename T>
    constexpr T& get(size_t index) {
        return std::get<std::vector<T>>(type_storage)[index];
    }
    
    // Traverse all objects of all types for serialization
    template <typename Visitor>
    constexpr void visit_all(Visitor&& visitor) {
        // For each type in the tuple
        [&]<size_t... Is>(std::index_sequence<Is...>) {
            // Visit each vector in the tuple
            (visit_vector<std::tuple_element_t<Is, std::tuple<std::vector<Ts>...>>>(
                std::get<Is>(type_storage), visitor
            ), ...);
        }(std::make_index_sequence<sizeof...(Ts)>{});
    }
    
private:
    // Visit all elements in a vector
    template <typename Vector, typename Visitor>
    constexpr void visit_vector(Vector& vec, Visitor&& visitor) {
        for (auto& item : vec) {
            visitor(item);
        }
    }
};
```

The requirement to know all types in advance forces a disciplined approach to constexpr computation design, where the type universe must be explicitly defined. This seeming limitation actually enables the yce machine's powerful capabilities by ensuring that all working memory can be properly managed and that the final results can be extracted and serialized into the result memory.

During the actual computation, the working memory managed by `multitype_storage` allows for complex data structures to be built and manipulated, while the explicit type tracking ensures that this data can be properly serialized into the compact, type-specific representation in the result memory.

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

**Solution:** The combination of `type_list`, `multitype_handler`, and `multitype_storage` provides a comprehensive framework for type-agnostic programming at compile-time.

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
