#include <type_traits>

struct fucking_allocator {};

// Forward declaration
template <typename T, typename Alloc = void> struct vector;
template <typename T, typename Alloc = void> struct set;

// Helper to propagate allocator ONLY into inner containers
template <typename T, typename Alloc> struct propagate_allocator {
  using type = T; // Base case: leave elements unchanged
};

template <template <typename, typename> typename Container, typename Elem,
          typename Alloc>
struct propagate_allocator<Container<Elem, void>, Alloc> {
  using type =
      Container<typename propagate_allocator<Elem, Alloc>::type, Alloc>;
};

template <typename T, typename Alloc>
using propagate_allocator_t = typename propagate_allocator<T, Alloc>::type;

// Containers
template <typename T, typename Alloc> struct vector {
  using allocator_type = Alloc;
  using value_type = T; // NEVER rewrite here!
};

template <typename T, typename Alloc> struct set {
  using allocator_type = Alloc;
  using value_type = T; // NEVER rewrite here!
};

// Test and proof
int main() {
  vector<set<vector<int>>, fucking_allocator> container;

  // allocator checks
  static_assert(
      std::is_same_v<decltype(container)::allocator_type, fucking_allocator>);
  static_assert(std::is_same_v<decltype(container)::value_type::allocator_type,
                               fucking_allocator>);
  static_assert(std::is_same_v<
                decltype(container)::value_type::value_type::allocator_type,
                fucking_allocator>);

  // element checks (value_type correctness)
  static_assert(
      std::is_same_v<decltype(container)::value_type::value_type::value_type,
                     int>);
}
