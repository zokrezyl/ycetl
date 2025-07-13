#pragma once

namespace ycetl {
// helpers to extract storage type from a container type
// this is used to calculato the storage type for all the nested containers
template <typename T, typename = void> struct storage_type_of {
  using type = T;
};

template <typename T>
struct storage_type_of<T, std::void_t<typename T::storage_type>> {
  using type = typename T::storage_type;
};

template <typename T>
using storage_type_of_t = typename storage_type_of<T>::type;

} // namespace ycetl
