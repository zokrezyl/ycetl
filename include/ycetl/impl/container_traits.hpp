#pragma once

namespace ycetl {
// helpers to extract storage type from a container type
// this is used to calculato the storage type for all the nested containers
//
//
// for types that do not declare the backend_type, the storage type is the type
// itself
template <typename T, typename = void> struct storage_type_of {
  using type = T;
};

// may be confusing, but we are using two notions here, storage_type and
// backend_type for nested containers, for instance from the outer containers
// perspective the storage type of inner T is not relevant, the inner container
// declares it as its own backend type
template <typename T>
struct storage_type_of<T, std::void_t<typename T::storage_type>> {
  using type = typename T::backend_type;
};

template <typename T>
using storage_type_of_t = typename storage_type_of<T>::type;

} // namespace ycetl
