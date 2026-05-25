// SPDX-License-Identifier: MIT

#include <tuple>
#include <type_traits>

// Type set for holding multiple types
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

// Example handler that stores values
template <typename T> struct storage_handler {
  constexpr void handle(const T &value) { stored_value = value; }

  constexpr T retrieve() const { return stored_value; }

private:
  T stored_value{};
};

// To use it:
constexpr int example() {
  using my_types = type_set<int, double>;
  multitype_handler<storage_handler, my_types> handler;

  // Access handlers directly
  handler.get_handler<int>().handle(42);
  handler.get_handler<double>().handle(3.14);

  // Retrieve values
  int i = handler.get_handler<int>().retrieve();       // i = 42
  double d = handler.get_handler<double>().retrieve(); // d = 3.14
  return 0;
}

int main() { constexpr auto res = example(); }
