
#include <ycetl/memory.hpp>

namespace ycetl {
template <typename T, typename Allocator> class list {
private:
  Allocator _alloc;
  ycetl::owned_pointer<storage_type> _storage;
}
