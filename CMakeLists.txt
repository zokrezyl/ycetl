cmake_minimum_required(VERSION 3.31.6)
project(yconstexpr)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # disables -std=gnu++20, enforces -std=c++20


include(cmake/CPM.cmake)

CPMAddPackage("gh:zokrezyl/ytrace-private#main")
CPMAddPackage("gh:yosh-matsuda/field-reflection@0.2.1")

#CPMAddPackage("gh:fmtlib/fmt#11.2.0") # is added by spdlog

add_library(yconstexpr INTERFACE)

target_include_directories(yconstexpr INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(yconstexpr INTERFACE ytrace ytrace::ytrace field_reflection::field_reflection)

add_library(yconstexpr::yconstexpr  ALIAS yconstexpr)

option(YCETL_BUILD_TESTS "Build unit tests" ON)
if(YCETL_BUILD_TESTS)
  enable_testing()
  add_subdirectory(unit_test)
endif()

option(YCETL_BUILD_EXAMPLES "Build examples" ON)
if(YCETL_BUILD_EXAMPLES)
  add_subdirectory(examples/yce)
endif()

# libclang-driven example: walks dawn/webgpu.h, emits a Python ctypes
# module and a constexpr C++ tree the consumer can walk at compile time.
# Off by default — only relevant when libclang is available.
option(YCETL_BUILD_WGPU_GLUE "Build the WebGPU clang→tree→python example" OFF)
if(YCETL_BUILD_WGPU_GLUE)
  add_subdirectory(examples/wgpu_glue)
endif()

