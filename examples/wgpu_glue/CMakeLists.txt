# wgpu_glue: walk a C header (dawn/webgpu.h here) with libclang, build an
# in-memory type tree, emit (a) a Python ctypes module and (b) a constexpr
# C++ header carrying the same tree as std::array initializers — the
# "ycetl-shape" tree the constexpr machinery can read.
#
# Default OFF so the normal build never requires libclang. Turn on with:
#   cmake -S . -B build-linux -DYCETL_BUILD_WGPU_GLUE=ON

cmake_minimum_required(VERSION 3.20)

# libclang location. Prefer Brew's LLVM 20 (headers + matching .so live
# together there), then fall back to system libclang-20 on Debian/Ubuntu.
set(_libclang_hints
    /home/linuxbrew/.linuxbrew/Cellar/llvm/20.1.6
    /home/linuxbrew/.linuxbrew/opt/llvm
    /usr/lib/llvm-20
    /usr/lib/llvm-19
    /usr/lib/llvm-18
)

find_path(LIBCLANG_INCLUDE_DIR clang-c/Index.h
    HINTS ${_libclang_hints}
    PATH_SUFFIXES include
)
find_library(LIBCLANG_LIBRARY
    NAMES clang libclang clang-20 libclang-20
    HINTS ${_libclang_hints}
    PATH_SUFFIXES lib lib/x86_64-linux-gnu
)

if(NOT LIBCLANG_INCLUDE_DIR OR NOT LIBCLANG_LIBRARY)
    message(WARNING
        "wgpu_glue: libclang not found "
        "(include=${LIBCLANG_INCLUDE_DIR} lib=${LIBCLANG_LIBRARY}) — skipping")
    return()
endif()

message(STATUS "wgpu_glue: libclang include = ${LIBCLANG_INCLUDE_DIR}")
message(STATUS "wgpu_glue: libclang lib     = ${LIBCLANG_LIBRARY}")

# Find the resource dir that matches the libclang we link against — it
# carries stddef.h / stdint.h / stdarg.h, which the parsed header needs
# the moment it does #include <stdint.h>. Without this, libclang prints
# "fatal error: 'stddef.h' file not found" (non-fatal in practice but
# noisy).
get_filename_component(_libclang_root "${LIBCLANG_LIBRARY}" DIRECTORY)
file(GLOB _resource_candidates "${_libclang_root}/clang/*")
set(WGPU_CLANG_RESOURCE_DIR "")
foreach(c ${_resource_candidates})
    if(EXISTS "${c}/include/stddef.h")
        set(WGPU_CLANG_RESOURCE_DIR "${c}")
        break()
    endif()
endforeach()
if(WGPU_CLANG_RESOURCE_DIR)
    message(STATUS "wgpu_glue: clang resource dir = ${WGPU_CLANG_RESOURCE_DIR}")
endif()

add_executable(wgpu_introspect wgpu_introspect.cpp)
target_include_directories(wgpu_introspect PRIVATE ${LIBCLANG_INCLUDE_DIR})
target_link_libraries(wgpu_introspect PRIVATE ${LIBCLANG_LIBRARY})
target_compile_definitions(wgpu_introspect PRIVATE
    WGPU_CLANG_RESOURCE_DIR="${WGPU_CLANG_RESOURCE_DIR}")

# Drive the tool against the bundled dawn/webgpu.h and produce both outputs
# at build time. Lands in the build dir so it's gitignored automatically.
set(_wgpu_header "${CMAKE_SOURCE_DIR}/test/include/dawn/webgpu.h")
set(_py_out      "${CMAKE_CURRENT_BINARY_DIR}/wgpu_ctypes.py")
set(_hpp_out     "${CMAKE_CURRENT_BINARY_DIR}/wgpu_tree.hpp")

add_custom_command(
    OUTPUT ${_py_out} ${_hpp_out}
    COMMAND $<TARGET_FILE:wgpu_introspect>
            --header  ${_wgpu_header}
            --python  ${_py_out}
            --tree    ${_hpp_out}
    DEPENDS wgpu_introspect ${_wgpu_header}
    COMMENT "wgpu_glue: walking ${_wgpu_header}"
    VERBATIM
)
add_custom_target(wgpu_glue_outputs ALL DEPENDS ${_py_out} ${_hpp_out})

# Tiny C++ consumer that #includes wgpu_tree.hpp and proves at compile
# time that the tree survived round-trip (via static_assert on counts and
# a couple of well-known enum/record names).
add_executable(wgpu_tree_check wgpu_tree_check.cpp)
target_include_directories(wgpu_tree_check PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(wgpu_tree_check wgpu_glue_outputs)

add_test(NAME wgpu_tree_check COMMAND wgpu_tree_check)

# Python-side smoke test: imports the generated ctypes module and
# checks well-known enum values + struct sizes against dawn/webgpu.h.
# Drops if python3 isn't on PATH (libclang is a much harder dep, so if
# we got this far we almost certainly have python3 too).
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME wgpu_ctypes_check
             COMMAND ${Python3_EXECUTABLE}
                     ${CMAKE_CURRENT_SOURCE_DIR}/wgpu_ctypes_check.py)
    set_tests_properties(wgpu_ctypes_check PROPERTIES
        ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}")
endif()
