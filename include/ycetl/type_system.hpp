#pragma once
#include <type_traits>
#include <ycetl/types.hpp>

namespace ycetl {

template <typename... Ts> struct type_set {
  template <template <typename...> typename T> using apply = T<Ts...>;
};

// Concatenates multiple type_sets into one.
template <typename...> struct type_set_concat;

// Base case: single type_set remains.
template <typename... Ts> struct type_set_concat<type_set<Ts...>> {
  using type = type_set<Ts...>;
};

// Concatenate two type_sets.
template <typename... Ts, typename... Us>
struct type_set_concat<type_set<Ts...>, type_set<Us...>> {
  using type = type_set<Ts..., Us...>;
};

// Recursive concatenation of multiple type_sets.
template <typename... Ts, typename... Us, typename... Rest>
struct type_set_concat<type_set<Ts...>, type_set<Us...>, Rest...> {
  using type = typename type_set_concat<type_set<Ts..., Us...>, Rest...>::type;
};

// Converts a single type into a type_set if it's not already one.
template <typename T> struct as_type_set {
  using type = type_set<T>;
};

// Specialization: Already a type_set.
template <typename... Ts> struct as_type_set<type_set<Ts...>> {
  using type = type_set<Ts...>;
};

// Helper alias to easily concatenate mixed types and type_sets.
template <typename... Args>
using type_set_concat_t =
    typename type_set_concat<typename as_type_set<Args>::type...>::type;

// Checks if a type T is contained within a type_set.
template <typename T, typename Set> struct contains;

// Base case: Empty set does not contain any type.
template <typename T> struct contains<T, type_set<>> : std::false_type {};

// Recursively checks if T is the same as U or contained in Rest.
template <typename T, typename U, typename... Rest>
struct contains<T, type_set<U, Rest...>>
    : std::conditional_t<std::is_same_v<T, U>, std::true_type,
                         contains<T, type_set<Rest...>>> {};

// Removes duplicate types from a type_set.
template <typename InputSet, typename Result = type_set<>>
struct remove_duplicates;

// Base case: No more input types left, return the Result.
template <typename Result> struct remove_duplicates<type_set<>, Result> {
  using type = Result;
};

// Recursive case: Process each type, adding it to Result if unique.
template <typename Head, typename... Tail, typename... Rs>
struct remove_duplicates<type_set<Head, Tail...>, type_set<Rs...>> {
  using type = typename remove_duplicates<
      type_set<Tail...>,
      std::conditional_t<contains<Head, type_set<Rs...>>::value,
                         type_set<Rs...>, type_set<Rs..., Head>>>::type;
};

// Helper alias to simplify usage of remove_duplicates.
template <typename Set>
using remove_duplicates_t = typename remove_duplicates<Set>::type;

// Extracts relevant associated types from a type (if available).
template <typename T, typename = void> struct relevant_types_of {
  using type = type_set<T>; // Default: the type itself is relevant.
};

// Specialization if T provides a `relevant_of` type member.
template <typename T>
struct relevant_types_of<T, std::void_t<typename T::relevant_of>> {
  using type = typename T::relevant_of;
};

// Recursively flattens relevant types, preventing cycles.
template <typename T, typename Seen = type_set<>, typename = void>
struct flatten_relevant_types {
  using type = type_set<T>; // Base case: no further relevant types.
};

// Specialization: Flatten types having `relevant_of` member.
template <typename T, typename Seen>
struct flatten_relevant_types<T, Seen, std::void_t<typename T::relevant_of>> {
  using immediate = typename T::relevant_of;
  using type = typename flatten_relevant_types<immediate, Seen>::type;
};

// Specialization: Flatten a type_set recursively.
template <typename... Ts, typename Seen>
struct flatten_relevant_types<type_set<Ts...>, Seen> {
  using type = remove_duplicates_t<
      type_set_concat_t<typename flatten_relevant_types<Ts, Seen>::type...>>;
};

// Cycle detection specialization: stop recursion if type already seen.
template <typename T, typename Seen>
struct flatten_relevant_types<T, Seen,
                              std::enable_if_t<contains<T, Seen>::value>> {
  using type = type_set<>; // Stop further recursion to prevent cycles.
};

// User-friendly alias to retrieve all relevant, flattened, unique types.
template <typename... Args>
using relevant_types_t = remove_duplicates_t<
    typename flatten_relevant_types<type_set<Args...>>::type>;

// Convenience alias to extract relevant types from a single type.
template <typename T>
using relevant_types_of_t = typename relevant_types_of<T>::type;

// type_set_tail: all but the first element
template <typename Set> struct type_set_tail;

// Empty and singleton cases yield an empty type_set
template <> struct type_set_tail<type_set<>> {
  using type = type_set<>;
};

template <typename T> struct type_set_tail<type_set<T>> {
  using type = type_set<>;
};

// General case: strip off the first type
template <typename T, typename... Rest>
struct type_set_tail<type_set<T, Rest...>> {
  using type = type_set<Rest...>;
};

// Alias for easier usage
template <typename Set>
using type_set_tail_t = typename type_set_tail<Set>::type;

// type_set_init: all but the last element
template <typename Set> struct type_set_init;

// Empty and singleton cases yield an empty type_set
template <> struct type_set_init<type_set<>> {
  using type = type_set<>;
};

template <typename T> struct type_set_init<type_set<T>> {
  using type = type_set<>;
};

// Recursive case: take the first element and append init of the rest
template <typename T, typename... Rest>
struct type_set_init<type_set<T, Rest...>> {
  using type = typename type_set_concat<
      type_set<T>, typename type_set_init<type_set<Rest...>>::type>::type;
};

// Alias for easier usage
template <typename Set>
using type_set_init_t = typename type_set_init<Set>::type;

// ---------------------- NEW: back (last element) ----------------------

// type_set_back: pick out the very last type in a non-empty type_set
template <typename Set> struct type_set_back;

// Base case: singleton holds the last element
template <typename T> struct type_set_back<type_set<T>> {
  using type = T;
};

// Recursive case: peel off head until only one remains
template <typename Head, typename... Rest>
struct type_set_back<type_set<Head, Rest...>> {
  using type = typename type_set_back<type_set<Rest...>>::type;
};

// Alias for easier usage
template <typename Set>
using type_set_back_t = typename type_set_back<Set>::type;

template <typename T> struct as_pointers;

template <typename... Ts> struct as_pointers<type_set<Ts...>> {
  using type = type_set<Ts *...>;
};

template <typename T> using as_pointers_t = typename as_pointers<T>::type;

// Example Usage:
using original = type_set<int, char, double>;
using pointers = as_pointers<original>;

template <template <typename> typename Wrapper, typename TypeSet>
struct apply_wrapper;

template <template <typename> typename Wrapper, typename... Ts>
struct apply_wrapper<Wrapper, type_set<Ts...>> {
  using type = type_set<Wrapper<Ts>...>;
};

template <template <typename> typename Wrapper, typename TypeSet>
using apply_wrapper_t = typename apply_wrapper<Wrapper, TypeSet>::type;

template <typename... Ts> struct template_arguments_t {
  template <template <typename...> typename T> using apply = T<Ts...>;
};

template <template <typename... Args> typename Template, typename... Ts>
class template_info {
public:
  template <typename... Args> using template_type = Template<Args...>;
  using template_arguments = template_arguments_t<Ts...>;
};

// --- NEW HELPER TRAITS FOR ROBUST SFINAE ---
// Helper to detect if T::template_type is a valid template alias
// It attempts to instantiate T::template_type to check if it's a template.
template <typename T, template <typename...> class = T::template_type>
struct has_template_type_alias_helper : std::true_type {};

template <typename T>
struct has_template_type_alias_helper<T, void> : std::false_type {
}; // Default for no match

template <typename T>
inline constexpr bool has_template_type_alias_v =
    has_template_type_alias_helper<T>::value;

// Helper to detect if T::template_arguments is a valid type alias
template <typename T, typename = void>
struct has_template_arguments_alias_helper : std::false_type {};

template <typename T>
struct has_template_arguments_alias_helper<
    T, std::void_t<typename T::template_arguments>> : std::true_type {};

template <typename T>
inline constexpr bool has_template_arguments_alias_v =
    has_template_arguments_alias_helper<T>::value;
// --- END NEW HELPER TRAITS ---

// Helper to detect if a type T exposes template_type and template_arguments
// This now uses the new, more robust helper traits.
template <typename T>
inline constexpr bool is_template_rebindable_v =
    (std::is_class_v<T> || std::is_union_v<T>) && // Must be a class/union
    has_template_type_alias_v<T> &&    // Check for template_type alias
    has_template_arguments_alias_v<T>; // Check for template_arguments alias

// Primary template for the rebind_template action.
// It takes the type to be rebound (T_Rebindable) and the new set of template
// arguments.
template <typename T_Rebindable, typename NewTypeSet,
          typename Enable = void> // Added Enable for SFINAE
struct rebind_template {};        // Not defined for arbitrary types

// Specialization for types that inherit from ycetl::template_info.
// This specialization is now enabled using the correct
// `is_template_rebindable_v` trait.
template <typename T_Rebindable, typename NewTypeSet>
struct rebind_template<T_Rebindable, NewTypeSet,
                       std::enable_if_t<ycetl::is_template_rebindable_v<
                           T_Rebindable>> // Use the trait directly
                       > {
  // Apply T_Rebindable's original template_type to the new arguments.
  // NewTypeSet is expected to have an 'apply' member that takes a template.
  // FIX: Pass T_Rebindable::template_type as a template, not a type.
  using type = typename NewTypeSet::template apply<
      T_Rebindable::template_type>; // Direct access, passing template
};

// Helper alias template for convenience.
template <typename T_Rebindable, typename NewTypeSet>
using rebind_template_t =
    typename rebind_template<T_Rebindable, NewTypeSet>::type;

// Trait to check if a type T has a nested 'memory_type' alias.
template <typename T, typename = void>
struct has_memory_type : std::false_type {};

template <typename T>
struct has_memory_type<
    T, std::void_t<std::enable_if_t<std::is_class_v<T> ||
                                    std::is_union_v<T>>, // Guard against
                                                         // primitives
                   typename T::memory_type>> : std::true_type {};

// Helper variable template for convenience.
template <typename T>
inline constexpr bool has_memory_type_v = has_memory_type<T>::value;

// Trait to check if a type is capable of having its memory type rebound.
// This requires:
// 1. The type exposes its template information (is_template_rebindable_v).
// 2. The type explicitly declares a 'memory_type' alias (has_memory_type_v).
template <typename T>
inline constexpr bool has_rebindable_memory_v =
    is_template_rebindable_v<T> && has_memory_type_v<T>;

// Alias template to rebind the memory type of a rebindable template.
// It assumes the memory type is the last argument in the template's argument
// list.
template <typename T_Rebindable, typename NewMemory>
using rebind_memory_t = rebind_template_t<
    T_Rebindable,
    ycetl::type_set_concat_t<
        ycetl::type_set_init_t<typename T_Rebindable::template_arguments>,
        NewMemory>>;

// helpers to extract storage type from a container type
// this is used to calculato the storage type for all the nested containers
//
//
// for types that do not declare the backend_type, the storage type is the type
// itself
template <typename T, typename = void> struct backend_type_of {
  using type = T;
};

// may be confusing, but we are using two notions here, backend_type and
// backend_type for nested containers, for instance from the outer containers
// perspective the storage type of inner T is not relevant, the inner container
// declares it as its own backend type
template <typename T>
struct backend_type_of<T, std::void_t<typename T::backend_type>> {
  using type = typename T::backend_type;
};

template <typename T>
using backend_type_of_t = typename backend_type_of<T>::type;

} // namespace ycetl
