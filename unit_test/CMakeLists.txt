cmake_minimum_required(VERSION 3.14)
project(test)

enable_testing()


if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
  message(FATAL_ERROR "Please configure with Ninja:  cmake -G Ninja …")
endif()


add_subdirectory(containers)
add_subdirectory(types)
add_subdirectory(concepts)
add_subdirectory(compiler)
