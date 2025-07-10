#include <type_traits>

template <typename, typename = void>
constexpr bool is_test_type = false;

template <typename T>
constexpr bool is_test_type<T, std::void_t<typename T::test_type>> = true;

// ───────────── Example usage ─────────────

struct Test1 {
  using test_type = Test1;
};

struct Test2 {
};

int main() {
  static_assert(is_test_type<Test1>, "Test1 defines test_type");
  static_assert(is_test_type<Test2>, "Test2 does NOT define test_type");
}


