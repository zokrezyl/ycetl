#pragma once

#include <ycetl/basic_string.hpp>

namespace ycetl {

#if 0
template <typename Memory =
              typename container::container_traits<char>::default_memory>
class string : public basic_string<char, std::char_traits<char>, Memory> {
  using basic_string<char, std::char_traits<char>, Memory>::basic_string;
  using relevant_of =
      typename basic_string<char, std::char_traits<char>, Memory>::relevant_of;
};
#else
template <
    typename Memory = typename container::container_traits<
        basic_string, type_set<char, std::char_traits<char>>>::default_memory>

using string = ycetl::basic_string<char, std::char_traits<char>,
                                   container::by_value, Memory>;

#endif

} // namespace ycetl
