#pragma once
#include <ycetl/memory.hpp>

namespace yectl {

template <typename CharT, typename Traits = std::char_traits<CharT>,
          typename Allocator = ycetl::default_allocator<CharT>>
class string {
public
  /* define all nested types */

private:
  Allocator _alloc;
  /* this pattern is again necessary if the string is to be nested in other
   * containers */
  ycetl::owned_pointer<ycetl::dynamic_vector<CharT>> _storage;

public:
  constexpr string() : _alloc(default_allocator<CharT>()), _storage() {}
  constexpr string(Allocator &alloc)
  /* add all constructors and methods by delegating most operations to _storage
   */
};

} // namespace yectl
