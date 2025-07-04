cmake_minimum_required(VERSION 3.14)
project(test)

enable_testing()


if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
  message(FATAL_ERROR "Please configure with Ninja:  cmake -G Ninja …")
endif()


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # disables -std=gnu++20, enforces -std=c++20

add_compile_options(-stdlib=libc++)

include(../cmake/CPM.cmake)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

#CPMAddPackage("gh:fmtlib/fmt#11.2.0") # is added by spdlog
#CPMAddPackage("gh:fmtlib/fmt#master") # is added by spdlog
CPMAddPackage("gh:gabime/spdlog@1.15.3")
#CPMAddPackage("gh:boost-ext/ut@2.3.1")
CPMAddPackage("gh:boost-ext/ut#master")
CPMAddPackage("gh:zokrezyl/ytrace-private#main")


file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")

add_compile_definitions(FMT_USE_CONSTEXPR)
add_compile_definitions(FMT_USE_CONSTEXPR_STRING)

foreach(test_file ${TEST_SOURCES})
  get_filename_component(test_name "${test_file}" NAME_WE)
  add_executable(${test_name} ${test_file})
  #target_link_libraries(${test_name} PRIVATE fmt::fmt Boost::ut)

  target_link_libraries(${test_name} PRIVATE Boost::ut ytrace::ytrace spdlog::spdlog)

  target_compile_options(${test_name} PRIVATE -include iostream)

  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

