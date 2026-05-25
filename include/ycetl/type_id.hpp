// SPDX-License-Identifier: MIT

#pragma once

#include <string_view>
#include <ycetl/hash.hpp>

namespace ycetl {
namespace type_id {
template <typename T> constexpr const char *_type_str_ptr() {
  constexpr const char *name = __PRETTY_FUNCTION__; // OK inside constexpr
  return name;
}

template <typename T> constexpr std::string_view _type_str() {
  return _type_str_ptr<T>();
}

template <typename T> constexpr std::string_view type_id() {
  return _type_str<T>();
}

template <typename T> constexpr size_t type_uid() {
  return ycetl::hash::fnv1a_hash(_type_str<T>());
}

class type_info {
public:
  std::string_view type_str;
  size_t hash;
};
} // namespace type_id
}; // namespace ycetl
