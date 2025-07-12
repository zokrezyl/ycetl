#pragma once

#include <ycetl/memory.hpp>

namespace ycetl {
template <typename Key, typename Compare, typename Allocator> class set {
private:
  Allocator _alloc;
  ycetl::owned_pointer<storage_type> _storage;
};

} // namespace ycetl
