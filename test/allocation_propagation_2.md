Original Problem Space and Desired Outcome:

The core problem is to ensure allocator propagation in nested container types within a generic container system that resembles STL containers. Specifically, when containers like vector, set, and unordered\_map are nested, the allocator type from the outermost container should automatically propagate and become the allocator for all inner containers. This propagation should occur without using type erasure or the traditional rebind allocator approach. The solution should rely purely on constexpr template metaprogramming in C++20.

STL-like Container Types:

The following STL-like container types should be considered:

* vector
* set
* unordered\_map
* map
* list
* unordered\_set
* deque

Each container type has specific template parameters typical to their STL counterparts:

* vector\<T, Allocator>
* set\<T, Compare, Allocator>
* unordered\_map\<Key, Value, Hash, KeyEqual, Allocator>
* map\<Key, Value, Compare, Allocator>
* list\<T, Allocator>
* unordered\_set\<T, Hash, KeyEqual, Allocator>
* deque\<T, Allocator>

The outer allocator:

The outer allocator refers to the allocator type specified at the outermost container instantiation level. This allocator must propagate automatically to inner nested containers. Thus, the nested containers should implicitly inherit the outer allocator type, ensuring consistent memory management across nested container levels without explicit user intervention or allocator rebinding.

Examples and Explanation:

Example 1:

vector\<vector<int>> nested\_vector;

In this case, the allocator type specified (or deduced) for the outer vector should automatically become the allocator type for the inner vector. The allocator is a multitype allocator that manages memory allocation based on the types used within the container.

Example 2:

unordered\_map\<int, vector\<set<int>>> complex\_nested\_container;

Here, the allocator used by unordered\_map should propagate automatically to the vector and then further propagate to the set. All nested types (unordered\_map, vector, and set) should implicitly share the same multitype allocator defined at the outermost level.

Challenge to Solve:

1. Write all necessary helper/meta-programming templates required for allocator propagation.
2. Write basic skeleton implementations (constructors, type definitions) for each mentioned container type (vector, set, unordered\_map, map, list, unordered\_set, deque) demonstrating how nested container types instantiate correctly with allocator propagation.
3. Write a constexpr-compatible main function demonstrating the allocator propagation concept explicitly. Include static\_assert statements to validate that the allocator of inner container types correctly inherits from the outermost container allocator, ensuring the allocator is of the multitype allocator type. Type erasure is prohibited; all validations must happen in a constexpr C++20 context.

