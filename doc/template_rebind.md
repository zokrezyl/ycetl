# Conceptual Summary: Template and Memory Rebinding

This document summarizes the core concepts and mechanisms discussed, implemented, and refined regarding template rebinding, memory management, and related helper structures in the `ycetl` library.

---

## 1. Template Rebind and Helpers

Template rebinding enables creating new template instances from existing template instantiations, by changing their type parameters.

* **Core utilities:**

  * `template_info`: Extracts and exposes template type information (template parameters).
  * `rebind_template_t`: Primary tool for template rebinding.

* **Use-case example:**

  ```cpp
  template <typename T>
  using rebind_vector_to_double = rebind_template_t<T, type_set<double>>;
  ```

## 2. Memory Rebind and Helpers

Memory rebinding provides a mechanism for changing the memory type of containers at compile-time, which is critical for nested containers to ensure type-consistent memory allocations.

* **Core utilities:**

  * `has_memory_type_v<T>`: Checks if a type exposes a memory type.
  * `has_rebindable_memory_v<T>`: Checks if a type can have its memory rebound.
  * `rebind_memory_t`: Explicitly rebinds the memory type of a rebindable template.

* **Use-case example:**

  ```cpp
  using vec_t = vector<vector<int>>;
  using downgraded_inner_memory = rebind_memory_t<vector<int>, new_memory_type>;
  ```

## 3. Multitype Handler

A `multitype_handler` manages multiple handler instances, each specialized for different types. This concept is particularly useful for type-based dispatch and allocation.

* **Core functionality:**

  * Stores handlers as a tuple (`std::tuple<HandlerImpl<T>...>`).
  * Provides direct access to handlers based on the handled type.

* **Use-case example:**

  ```cpp
  multitype_handler<allocator, type_set<int, double>> handlers;
  auto& int_allocator = handlers.get_handler<int>();
  ```

## 4. Multitype Memory

`multitype_memory` builds upon `multitype_handler` to provide type-specific memory allocation strategies. It explicitly leverages `trivial_shared_ptr` for shared ownership and automatic reference counting.

* **Core functionality:**

  * Allocates memory via type-specific memory backends.
  * Uses shared pointers (`trivial_shared_ptr`) to manage memory backends.

* **Use-case example:**

  ```cpp
  multitype_memory<dummy_backend, type_set<int, double>> memory;
  int* ptr = memory.allocate<int>(5);
  memory.deallocate(ptr);
  ```

## 5. Handler and Memory Downgrade

Downgrading allows creation of a smaller (subset) handler or memory manager from an existing larger one, ensuring reuse and consistency of resources.

* **Core functionality:**

  * Explicit downgrade constructors allow for creating smaller instances from larger ones.
  * Maintains reference sharing, avoiding unnecessary resource duplication.

* **Use-case example:**

  ```cpp
  multitype_memory<dummy_backend, type_set<int, double>> original_memory;
  multitype_memory<dummy_backend, type_set<int>> downgraded_memory(original_memory);
  ```

## Additional Covered Concepts:

* **`type_set` and helpers:**

  * `type_set_init_t`: Selects all but the last element of a `type_set`.
  * `type_set_concat_t`: Merges multiple `type_set` instances.
  * `apply_wrapper_t`: Applies wrappers (e.g., smart pointers) to type sets.

* **Reference counting:**

  * Explicitly managed via `trivial_shared_ptr`, ensuring resource safety and shared ownership semantics.

---

This conceptual overview covers the primary structures and their interactions, providing clarity on the template and memory management strategies implemented within the `ycetl` library.

