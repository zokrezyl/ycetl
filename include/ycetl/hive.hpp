
#include <ycetl/memory.hpp>

namespace ycetl {
template <typename T, typename Memory> class list {
private:
  Memory _alloc;
  storage_type _storage;
}
