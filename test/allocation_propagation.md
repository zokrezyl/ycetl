# Allocator Propagation Problem in Nested Containers

## Original Problem:

We have STL-like container templates designed similarly to the standard C++ containers (e.g., vector, set, unordered\_map, etc.). Each container template accepts two primary template parameters:

* **Element Type(s)**
* **Allocator**

Currently, when containers are nested (e.g., `set<vector<int>>`), each inner container defaults to using its own allocator rather than inheriting the allocator from the outer container.

## Desired Outcome:

We want a template-based mechanism that ensures allocator propagation, meaning that when containers are nested, all inner containers automatically inherit and use the allocator specified by the outermost container.

## Container Types:

The following STL-like container types are considered:

* vector\<T, Allocator>
* set\<Key, Allocator>
* unordered\_map\<Key, Value, Allocator>
* map\<Key, Value, Allocator>
* unordered\_set\<Key, Allocator>
* list\<T, Allocator>
* deque\<T, Allocator>

## Examples:

Example 1:

```cpp
set<vector<int>> s1;
```

Expected behavior:

* The allocator type specified or deduced for `set` propagates into the nested `vector<int>`, ensuring both the `set` and its inner `vector` use the identical allocator type.

Example 2:

```cpp
unordered_map<int, vector<int>> m1;
```

Expected behavior:

* The allocator specified or deduced for `unordered_map` propagates automatically into the nested container `vector<int>`.

## Explanation of "Outer Allocator":

In nested containers, the "outer allocator" refers to the allocator specified by the outermost container type. For example, in the nested type:

```cpp
set<vector<int>>
```

the allocator provided (implicitly or explicitly) for the outer container (`set`) is considered the "outer allocator." The goal is that all inner containers (such as the nested `vector<int>`) automatically inherit and use this same allocator type. Thus, the allocator propagates inward through all nested container types.

