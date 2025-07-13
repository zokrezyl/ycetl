#include <ycetl/relevant_types.hpp>
#include <ycetl/memory.hpp>

#include <vector>

template <typename... Args>
struct container_traits {
  using value_type = typename ycetl::relevant_types_t<Args...>;
  using allocator_type = ycetl::memory::multitype_allocator<
      ycetl::memory::dynamic_allocator, value_type>;
};

template <typename... Args>
struct container {
  using traits = container_traits<Args...>;
  using default_allocator = typename traits::allocator_type;
};

// Example STL-like containers
template <typename T,
          typename Allocator = typename container<T>::default_allocator>
struct vector : public container<T, Allocator> {
  using value_type = T;
};

template <typename T,
          typename Allocator = typename container<T>::default_allocator>
struct set : public container<T, Allocator> {
  using value_type = T;
};

template <typename Key, typename Value,
          typename Allocator = typename container<Key, Value>::default_allocator>
struct unordered_map : public container<Key, Value, Allocator> {
  using key_type = Key;
  using mapped_type = Value;
};


template <template <typename... Args> typename shit>
class  test {
public:
  template <typename ...Args>
  using my_shit = shit<Args...>;

};

int main() {
  test<std::vector<std::vector<int>>> t;
  typename decltype(t)::template my_shit<int> v; // vector<int>
}

int _main() {
  set<vector<int>> s1;
  unordered_map<int, vector<int>> m1;

}

