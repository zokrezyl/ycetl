// SPDX-License-Identifier: MIT

#pragma once
#include <cxxabi.h>

namespace ycetl {
namespace print {

std::string demangle(const char *name) {
  int status = 0;
  char *realname = abi::__cxa_demangle(name, nullptr, nullptr, &status);
  std::string result = (status == 0 && realname) ? realname : name;
  std::free(realname);
  return result;
}

template <typename... Ts> void print() {
  // fold-expression to print each type
  bool first = true;
  ((std::cout << (first ? "" : ", ") << demangle(typeid(Ts).name()),
    first = false),
   ...);
  std::cout << "\n";
}

} // namespace print
} // namespace ycetl
