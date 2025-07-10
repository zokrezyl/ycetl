Y Const Expression Template Library

# The yce machine: A Comprehensive Guide

**Document Date:** 2025-07-10 09:04:35  
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

The `multitype_handler` provides mechanisms to operate uniformly across different types, enabling the implementation of type-agnostic algorithms in the constexpr context. It uses template metaprogramming techniques to apply operations across the types in a `type_list`.

```cpp
template <template <typename> class Operation, typename TypeList>
struct multitype_handler;

template <template <typename> class Operation, typename... Ts>
struct multitype_handler<Operation, type_list<Ts...>> {
    // Apply operation to each type
    using results = type_list<typename Operation<Ts>::type...>;
    
    // Other utilities for handling multiple types
    // ...
};
```

### multitype_storage

The `multitype_storage` component addresses one of the most significant challenges in constexpr computation: storing and manipulating heterogeneous data at compile-time. It serves as the backbone of the working memory system, tracking allocations of different types throughout the computation chain and ensuring proper cleanup.

```cpp
template <typename TypeList>
class multitype_storage;

template <typename... Ts>
class multitype_storage<type_list<Ts...>> {
    // Storage for each type in the list
    std::tuple<std::vector<Ts>...> type_storage;

public:
    template <typename T>
    constexpr T* allocate() {
        // Allocate an object of type T and track it
        auto& storage = get_storage<T>();
        storage.push_back(T{});
        return &storage.back();
    }
    
    template <typename T>
    constexpr void deallocate(T* ptr) {
        // Find and deallocate the object
        // (simplified - actual implementation is more complex)
    }
    
    // Clean up all allocations when storage goes out of scope
    constexpr ~multitype_storage() {
        // Automatic cleanup - critical for constexpr compliance
    }
    
private:
    template <typename T>
    constexpr auto& get_storage() {
        return std::get<std::vector<T>>(type_storage);
    }
};
```

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
