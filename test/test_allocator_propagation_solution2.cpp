#include <type_traits>
#include <vector>
#include <set>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <list>
#include <deque>

// Primary template (base case for non-container types)
template <typename T, typename Allocator>
struct propagate_allocator {
  using type = T;
};

// Specializations for containers to propagate allocator recursively:

// vector specialization
template <typename T, typename OldAllocator, typename Allocator>
struct propagate_allocator<std::vector<T, OldAllocator>, Allocator> {
  using type = std::vector<
    typename propagate_allocator<T, Allocator>::type,
    Allocator
  >;
};

// set specialization
template <typename T, typename Compare, typename OldAllocator, typename Allocator>
struct propagate_allocator<std::set<T, Compare, OldAllocator>, Allocator> {
  using type = std::set<
    typename propagate_allocator<T, Allocator>::type,
    Compare,
    Allocator
  >;
};

// unordered_set specialization
template <typename T, typename Hash, typename KeyEqual, typename OldAllocator, typename Allocator>
struct propagate_allocator<std::unordered_set<T, Hash, KeyEqual, OldAllocator>, Allocator> {
  using type = std::unordered_set<
    typename propagate_allocator<T, Allocator>::type,
    Hash,
    KeyEqual,
    Allocator
  >;
};

// unordered_map specialization
template <typename Key, typename Value, typename Hash, typename KeyEqual, typename OldAllocator, typename Allocator>
struct propagate_allocator<std::unordered_map<Key, Value, Hash, KeyEqual, OldAllocator>, Allocator> {
  using type = std::unordered_map<
    typename propagate_allocator<Key, Allocator>::type,
    typename propagate_allocator<Value, Allocator>::type,
    Hash,
    KeyEqual,
    Allocator
  >;
};

// map specialization
template <typename Key, typename Value, typename Compare, typename OldAllocator, typename Allocator>
struct propagate_allocator<std::map<Key, Value, Compare, OldAllocator>, Allocator> {
  using type = std::map<
    typename propagate_allocator<Key, Allocator>::type,
    typename propagate_allocator<Value, Allocator>::type,
    Compare,
    Allocator
  >;
};

// list specialization
template <typename T, typename OldAllocator, typename Allocator>
struct propagate_allocator<std::list<T, OldAllocator>, Allocator> {
  using type = std::list<
    typename propagate_allocator<T, Allocator>::type,
    Allocator
  >;
};

// deque specialization
template <typename T, typename OldAllocator, typename Allocator>
struct propagate_allocator<std::deque<T, OldAllocator>, Allocator> {
  using type = std::deque<
    typename propagate_allocator<T, Allocator>::type,
    Allocator
  >;
};

// Helper alias template
template <typename Container, typename Allocator>
using propagate_allocator_t = typename propagate_allocator<Container, Allocator>::type;

// Example allocator (user-defined)
template <typename T>
struct MyAllocator {
  using value_type = T;

  constexpr MyAllocator() noexcept = default;

  template <typename U>
  constexpr MyAllocator(const MyAllocator<U>&) noexcept {}

  constexpr T* allocate(std::size_t n) { return static_cast<T*>(::operator new(n * sizeof(T))); }
  constexpr void deallocate(T* p, std::size_t) noexcept { ::operator delete(p); }

  template <typename U>
  struct rebind { using other = MyAllocator<U>; };
};

// constexpr-compatible main demonstrating allocator propagation
int main() {
  using OuterAllocator = MyAllocator<int>;

  using NestedVector = propagate_allocator_t<std::vector<std::vector<int>>, OuterAllocator>;
  static_assert(std::is_same_v<NestedVector, std::vector<std::vector<int, OuterAllocator>, OuterAllocator>>);

  using ComplexNested = propagate_allocator_t<
    std::unordered_map<int, std::vector<std::set<int>>>,
    OuterAllocator
  >;

  static_assert(std::is_same_v<
    ComplexNested,
    std::unordered_map<
      int,
      std::vector<std::set<int, std::less<int>, OuterAllocator>, OuterAllocator>,
      std::hash<int>,
      std::equal_to<int>,
      OuterAllocator
    >
  >);

  return 0;
}

