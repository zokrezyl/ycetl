// SPDX-License-Identifier: MIT

// Round-trip proof: include the generated header and walk the tree at
// compile time, then print a short summary at runtime so the test target
// produces something meaningful.

#include <iostream>
#include <string_view>

#include "wgpu_tree.hpp"

namespace {

// constexpr name lookup against the homogeneous name index.
constexpr int find_record(std::string_view name) {
  for (std::size_t i = 0; i < wgpu_tree::records.size(); ++i)
    if (wgpu_tree::records[i].name == name)
      return int(i);
  return -1;
}

constexpr int find_enum(std::string_view name) {
  for (std::size_t i = 0; i < wgpu_tree::enums.size(); ++i)
    if (wgpu_tree::enums[i].name == name)
      return int(i);
  return -1;
}

} // namespace

int main() {
  // These are stable identities in dawn/webgpu.h. If the parse ever
  // silently regresses to "nothing found" we'll catch it at compile
  // time.
  static_assert(wgpu_tree::record_count > 50,
                "expected dawn webgpu.h to declare >50 records");
  static_assert(wgpu_tree::enum_count > 40,
                "expected dawn webgpu.h to declare >40 enums");

  static_assert(find_enum("WGPUTextureFormat") >= 0,
                "WGPUTextureFormat must be present");
  static_assert(find_record("WGPUBindGroupEntry") >= 0,
                "WGPUBindGroupEntry must be present");

  std::cout << "wgpu_tree: records=" << wgpu_tree::record_count
            << " enums=" << wgpu_tree::enum_count
            << " typedefs=" << wgpu_tree::typedef_count
            << " functions=" << wgpu_tree::function_count << '\n';
  std::cout << "  WGPUTextureFormat is enum #" << find_enum("WGPUTextureFormat")
            << '\n';
  std::cout << "  WGPUBindGroupEntry is record #"
            << find_record("WGPUBindGroupEntry") << '\n';
  return 0;
}
