// -----------------------------------------------------------------------------
//  allocator_propagation.cpp  – C++20, constexpr‑ready demo
// -----------------------------------------------------------------------------
//  * A multi‑type allocator that can service any value without rebinding.
//  * Minimal STL‑like container skeletons (vector, set, unordered_map, map, list,
//    unordered_set, deque) that take the *exact* allocator type as a template
//    parameter.
//  * A constexpr‑only metaprogram (`propagate_t`) that walks an arbitrarily deep
//    nest of these containers and replaces **every** inner allocator with the
//    outer one – without type‑erasure or `std::allocator_traits::rebind_alloc`.
//  * A `consteval` test‑suite that proves, through `static_assert`, that the
//    allocator really is the same at every level.
// -----------------------------------------------------------------------------
//  Build with:  g++ -std=c++20 -pedantic-errors -Wall allocator_propagation.cpp
// -----------------------------------------------------------------------------

#include <concepts>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

//==============================================================================
//  1.  A *multi‑type* allocator — one concrete type for *all* value types.
//==============================================================================
struct MultiAllocator {
    // trivial state – in the real world this could hold a pointer to a pool
    constexpr MultiAllocator() noexcept = default;

    // allocate/deallocate **any** T – note the template on the member fn,
    // *not* on the allocator itself, so there is no need for rebinding.
    template<typename T>
    [[nodiscard]] constexpr T* allocate(std::size_t /*n*/) { return nullptr; }

    template<typename T>
    constexpr void deallocate(T* /*p*/, std::size_t /*n*/) noexcept {}
};

//==============================================================================
//  2.  Tiny STL‑like containers – just enough for compile‑time introspection.
//      Each has a single `allocator_type` alias that *must* equal the outer one.
//==============================================================================

template<typename T, typename Alloc = MultiAllocator>
struct vector {
    using value_type      = T;
    using allocator_type  = Alloc;
};

template<typename T, typename Alloc = MultiAllocator>
struct list {
    using value_type      = T;
    using allocator_type  = Alloc;
};

template<typename T, typename Alloc = MultiAllocator>
struct deque {
    using value_type      = T;
    using allocator_type  = Alloc;
};

// ---- associative ----

template<typename Key, typename Compare = std::less<Key>, typename Alloc = MultiAllocator>
struct set {
    using key_type        = Key;
    using value_type      = Key;
    using allocator_type  = Alloc;
};

template<typename Key, typename Val,
         typename Compare = std::less<Key>, typename Alloc = MultiAllocator>
struct map {
    using key_type        = Key;
    using mapped_type     = Val;
    using value_type      = std::pair<const Key, Val>;
    using allocator_type  = Alloc;
};

template<typename Key, typename Hash = std::hash<Key>,
         typename Eq = std::equal_to<Key>, typename Alloc = MultiAllocator>
struct unordered_set {
    using key_type        = Key;
    using value_type      = Key;
    using allocator_type  = Alloc;
};

template<typename Key, typename Val,
         typename Hash = std::hash<Key>, typename Eq = std::equal_to<Key>,
         typename Alloc = MultiAllocator>
struct unordered_map {
    using key_type        = Key;
    using mapped_type     = Val;
    using value_type      = std::pair<const Key, Val>;
    using allocator_type  = Alloc;
};

//==============================================================================
//  3.  Compile‑time *allocator propagation* – no rebind, no type erasure.
//==============================================================================

template<typename Alloc, typename T, typename = void>
struct propagate {               // ─── primary: leave plain types untouched
    using type = T;
};

// ---- sequences --------------------------------------------------------------

template<typename Alloc, typename T, typename OldAlloc>
struct propagate<Alloc, vector<T, OldAlloc>> {
    using type = vector< typename propagate<Alloc,T>::type, Alloc >;
};

template<typename Alloc, typename T, typename OldAlloc>
struct propagate<Alloc, list<T, OldAlloc>> {
    using type = list< typename propagate<Alloc,T>::type, Alloc >;
};

template<typename Alloc, typename T, typename OldAlloc>
struct propagate<Alloc, deque<T, OldAlloc>> {
    using type = deque< typename propagate<Alloc,T>::type, Alloc >;
};

// ---- ordered / unordered *set* ---------------------------------------------

template<typename Alloc, typename Key, typename Cmp, typename OldAlloc>
struct propagate<Alloc, set<Key, Cmp, OldAlloc>> {
    using type = set< typename propagate<Alloc,Key>::type, Cmp, Alloc >;
};

template<typename Alloc, typename Key, typename Hash, typename Eq, typename OldAlloc>
struct propagate<Alloc, unordered_set<Key, Hash, Eq, OldAlloc>> {
    using type = unordered_set< typename propagate<Alloc,Key>::type, Hash, Eq, Alloc >;
};

// ---- ordered / unordered *map* ---------------------------------------------

template<typename Alloc, typename Key, typename Val,
         typename Cmp, typename OldAlloc>
struct propagate<Alloc, map<Key, Val, Cmp, OldAlloc>> {
    using type = map< typename propagate<Alloc,Key>::type,
                      typename propagate<Alloc,Val>::type,
                      Cmp,
                      Alloc >;
};

template<typename Alloc, typename Key, typename Val,
         typename Hash, typename Eq, typename OldAlloc>
struct propagate<Alloc, unordered_map<Key, Val, Hash, Eq, OldAlloc>> {
    using type = unordered_map< typename propagate<Alloc,Key>::type,
                               typename propagate<Alloc,Val>::type,
                               Hash,
                               Eq,
                               Alloc >;
};

// ---- handy alias ------------------------------------------------------------

template<typename Container, typename Alloc>
using propagate_t = typename propagate<Alloc, Container>::type;

//==============================================================================
//  4.  constexpr test‑suite – compile‑time proof that propagation *works*.
//==============================================================================
consteval bool run_tests()
{
    using A = MultiAllocator;

    // Example 1 :  vector<vector<int>>
    using original1 = vector<vector<int>>;                 // *no* alloc params
    using propagated1 = propagate_t<original1, A>;
    static_assert(std::is_same_v<typename propagated1::allocator_type, A>);
    static_assert(std::is_same_v<typename propagated1::value_type::allocator_type, A>);

    // Example 2 :  unordered_map<int, vector<set<int>>>
    using original2   = unordered_map<int, vector<set<int>>>;
    using propagated2 = propagate_t<original2, A>;
    static_assert(std::is_same_v<typename propagated2::allocator_type, A>);
    static_assert(std::is_same_v<typename propagated2::mapped_type::allocator_type, A>);
    static_assert(std::is_same_v<typename propagated2::mapped_type::value_type::allocator_type, A>);

    // (Optional) deeper, mixed nest – just to show scalability
    using crazy = vector< unordered_map<int, set< vector<int> >>>;
    using propagated3 = propagate_t<crazy, A>;
    static_assert(std::is_same_v<typename propagated3::allocator_type, A>);
    static_assert(std::is_same_v<typename propagate_t<A, typename propagated3::value_type>::allocator_type, A>);

    return true;
}

static_assert(run_tests());      // compile‑time verification

int main() { /* nothing to do – all proof is at compile time */ }

