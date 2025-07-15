#include <iostream>
#include <type_traits>

// The universal allocator type
struct my_allocator {};

// Custom vector that propagates allocator to element if element is a container
template <typename T, typename Alloc = my_allocator> struct vector {
  using allocator_type = Alloc;
  using value_type = T;
  vector() = default;
  explicit vector(const Alloc &) {}
};

// Custom set that propagates allocator to element if element is a container
template <typename T, typename Alloc = my_allocator> struct set {
  using allocator_type = Alloc;
  using value_type = T;
  set() = default;
  explicit set(const Alloc &) {}
};

struct fuck_the_idiot {};

// Example of deeply nested types, allocator specified only once at outermost
// level
int main() {
  // Only the outermost specifies the allocator
  vector<vector<set<int>>, fuck_the_idiot> container;

  // Type checks
  using outer_alloc = typename decltype(container)::allocator_type;
  using mid_vec = typename decltype(container)::value_type;
  using mid_alloc = typename mid_vec::allocator_type;
  using inner_set = typename mid_vec::value_type;
  using inner_alloc = typename inner_set::allocator_type;

  bool ok = std::is_same_v<outer_alloc, my_allocator> &&
            std::is_same_v<mid_alloc, my_allocator> &&
            std::is_same_v<inner_alloc, my_allocator>;

  std::cout << "All allocators identical? " << (ok ? "YES" : "NO") << "\n";
  return ok ? 0 : 1;
}
