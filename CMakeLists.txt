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

