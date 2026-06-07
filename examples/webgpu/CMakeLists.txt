# webgpu: walk a C header (one of the <impl>/include/webgpu.h here) with
# libclang, build an in-memory type tree, emit (a) a Python ctypes module and
# (b) a constexpr C++ header carrying the same tree as std::array initializers
# — the "ycetl-shape" tree the constexpr machinery can read.
#
# Default OFF so the normal build never requires libclang. Turn on with:
#   cmake -S . -B build-linux -DYCETL_BUILD_WEBGPU=ON
#
# Pick which implementation's header to parse with -DWEBGPU_IMPL=dawn|native
# (the subdirectory under this folder that carries include/webgpu.h).

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
        "webgpu: libclang not found "
        "(include=${LIBCLANG_INCLUDE_DIR} lib=${LIBCLANG_LIBRARY}) — skipping")
    return()
endif()

message(STATUS "webgpu: libclang include = ${LIBCLANG_INCLUDE_DIR}")
message(STATUS "webgpu: libclang lib     = ${LIBCLANG_LIBRARY}")

# Find the resource dir that matches the libclang we link against — it
# carries stddef.h / stdint.h / stdarg.h, which the parsed header needs
# the moment it does #include <stdint.h>. Without this, libclang prints
# "fatal error: 'stddef.h' file not found" (non-fatal in practice but
# noisy).
get_filename_component(_libclang_root "${LIBCLANG_LIBRARY}" DIRECTORY)
file(GLOB _resource_candidates "${_libclang_root}/clang/*")
set(WEBGPU_CLANG_RESOURCE_DIR "")
foreach(c ${_resource_candidates})
    if(EXISTS "${c}/include/stddef.h")
        set(WEBGPU_CLANG_RESOURCE_DIR "${c}")
        break()
    endif()
endforeach()
if(WEBGPU_CLANG_RESOURCE_DIR)
    message(STATUS "webgpu: clang resource dir = ${WEBGPU_CLANG_RESOURCE_DIR}")
endif()

add_executable(webgpu_introspect webgpu_introspect.cpp)
target_include_directories(webgpu_introspect PRIVATE ${LIBCLANG_INCLUDE_DIR})
target_link_libraries(webgpu_introspect PRIVATE ${LIBCLANG_LIBRARY})
target_compile_definitions(webgpu_introspect PRIVATE
    WEBGPU_CLANG_RESOURCE_DIR="${WEBGPU_CLANG_RESOURCE_DIR}")

# Which implementation's header to parse. Each impl lives in its own
# subdirectory carrying include/webgpu.h (dawn/, native/, …). The tool
# itself is impl-agnostic, so it's always built; only the codegen + tests
# below need an actual header.
set(WEBGPU_IMPL "dawn" CACHE STRING
    "WebGPU header implementation subdir to parse: dawn or native")
set(_webgpu_header "${CMAKE_CURRENT_SOURCE_DIR}/${WEBGPU_IMPL}/include/webgpu.h")
if(NOT EXISTS "${_webgpu_header}")
    message(WARNING
        "webgpu: no header for impl '${WEBGPU_IMPL}' at ${_webgpu_header} — "
        "built webgpu_introspect only, skipping codegen + tests")
    return()
endif()
message(STATUS "webgpu: impl = ${WEBGPU_IMPL}  header = ${_webgpu_header}")

# Drive the tool against the selected webgpu.h and produce both outputs at
# build time. Lands in the build dir so it's gitignored automatically.
set(_py_out  "${CMAKE_CURRENT_BINARY_DIR}/webgpu_ctypes.py")
set(_hpp_out "${CMAKE_CURRENT_BINARY_DIR}/webgpu_tree.hpp")

add_custom_command(
    OUTPUT ${_py_out} ${_hpp_out}
    COMMAND $<TARGET_FILE:webgpu_introspect>
            --header  ${_webgpu_header}
            --python  ${_py_out}
            --tree    ${_hpp_out}
    DEPENDS webgpu_introspect ${_webgpu_header}
    COMMENT "webgpu: walking ${_webgpu_header}"
    VERBATIM
)
add_custom_target(webgpu_outputs ALL DEPENDS ${_py_out} ${_hpp_out})

# Tiny C++ consumer that #includes webgpu_tree.hpp and proves at compile
# time that the tree survived round-trip (via static_assert on counts and
# a couple of well-known enum/record names).
add_executable(webgpu_tree_check webgpu_tree_check.cpp)
target_include_directories(webgpu_tree_check PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(webgpu_tree_check webgpu_outputs)

add_test(NAME webgpu_tree_check COMMAND webgpu_tree_check)

# The ycetl path: a constexpr program that #includes webgpu_tree.hpp and
# rebuilds the Python bindings FROM the tree using ycetl's constexpr string
# (working memory) baked into a std::array (result memory). Its output must be
# byte-identical to the direct emit_python path. Built only on demand (the
# constexpr evaluation of ~150 KB is heavy), so it's EXCLUDE_FROM_ALL.
add_executable(webgpu_emit_py EXCLUDE_FROM_ALL webgpu_emit_py.cpp)
target_include_directories(webgpu_emit_py PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(webgpu_emit_py PRIVATE yconstexpr::yconstexpr)
add_dependencies(webgpu_emit_py webgpu_outputs)   # needs the generated tree
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(webgpu_emit_py PRIVATE -fconstexpr-steps=2000000000)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(webgpu_emit_py PRIVATE
        -fconstexpr-ops-limit=2000000000 -fconstexpr-loop-limit=10000000)
endif()

set(_from_tree "${CMAKE_CURRENT_BINARY_DIR}/webgpu_ctypes_from_tree.py")
add_custom_command(
    OUTPUT ${_from_tree}
    COMMAND $<TARGET_FILE:webgpu_emit_py> ${_from_tree}
    DEPENDS webgpu_emit_py
    COMMENT "webgpu: ycetl re-emitting the bindings from the constexpr tree"
    VERBATIM)
add_custom_target(webgpu_from_tree DEPENDS ${_from_tree})

# Python-side smoke test: imports the generated ctypes module and
# checks well-known enum values + struct sizes against webgpu.h.
# Drops if python3 isn't on PATH (libclang is a much harder dep, so if
# we got this far we almost certainly have python3 too).
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME webgpu_ctypes_check
             COMMAND ${Python3_EXECUTABLE}
                     ${CMAKE_CURRENT_SOURCE_DIR}/${WEBGPU_IMPL}/webgpu_ctypes_check.py)
    set_tests_properties(webgpu_ctypes_check PROPERTIES
        ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}")
endif()
