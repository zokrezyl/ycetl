// SPDX-License-Identifier: MIT

// Storage handler that manages fixed allocations in a constexpr context
#include <tuple>
template <typename... Ts> struct type_set {
  template <typename NewT> static constexpr auto add() {
    return type_set<Ts..., NewT>{};
  }

  static constexpr std::size_t size = sizeof...(Ts);
};

// Simplified multitype_handler - HandlerImpl first for easier specialization
template <template <typename> class HandlerImpl, typename TypeSet>
class multitype_handler;

template <template <typename> class HandlerImpl, typename... Ts>
class multitype_handler<HandlerImpl, type_set<Ts...>> {
private:
  // Tuple of handlers for each type in the type_set
  std::tuple<HandlerImpl<Ts>...> handlers_;

public:
  // Default constructor
  constexpr multitype_handler() = default;

  // Access handler for a specific type
  template <typename T> constexpr HandlerImpl<T> &get_handler() {
    return std::get<HandlerImpl<T>>(handlers_);
  }

  template <typename T> constexpr const HandlerImpl<T> &get_handler() const {
    return std::get<HandlerImpl<T>>(handlers_);
  }
};

template <typename T, std::size_t Capacity = 1000> struct storage {
  // Allocate a T object from the internal pool
  constexpr T *allocate() {
    if (count >= Capacity)
      return nullptr;
    T *ptr = &data[count];
    allocations[count] = ptr;
    count++;
    return ptr;
  }

  // Handle method to allocate and initialize an object
  constexpr void handle(const T &value) {
    if (T *ptr = allocate()) {
      *ptr = value;
    }
  }

  // Get count of allocated objects
  constexpr std::size_t allocated_count() const { return count; }

  // Get pointer to allocated object by index
  constexpr T *get(std::size_t index) const {
    if (index < count)
      return allocations[index];
    return nullptr;
  }

private:
  std::array<T, Capacity> data{};
  std::array<T *, Capacity> allocations{};
  std::size_t count = 0;
};

// Example constexpr function demonstrating usage
constexpr int test() {
  using my_types = type_set<int, double, char>;
  multitype_handler<storage, my_types> handler;

  // Store some values
  handler.get_handler<int>().handle(42);
  handler.get_handler<double>().handle(3.14);
  handler.get_handler<char>().handle('A');

  // Verify storage
  if (int *int_ptr = handler.get_handler<int>().get(0)) {
    return *int_ptr; // Return 42
  }
  return 0;
}

// Main function using the constexpr result
int main() {
  constexpr auto res = test();
  static_assert(res == 42, "Should return allocated value (42)");
  return 0;
}
