cmake_minimum_required(VERSION 3.14)
project(test)

enable_testing()


if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
  message(FATAL_ERROR "Please configure with Ninja:  cmake -G Ninja …")
endif()


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

include(../../cmake/CPM.cmake)
CPMAddPackage("gh:boost-ext/ut#master")
CPMAddPackage("gh:zokrezyl/ytrace-private#b948bff5f656d9abe156abad3106f6fd0c758c9a")


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

