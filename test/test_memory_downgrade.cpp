#include <ycetl/memory.hpp>



constexpr int test() {
  
  using original_ts = type_set<int, double, char>;

  using original_memory_ts = default_memory<original_ts>;

  auto original_memory = original_memory_ts{};

  using downgraded_ts = type_set<int, char>;

  auto downgraded_memory = original_memory.downgrade<downgraded_ts>();



}
