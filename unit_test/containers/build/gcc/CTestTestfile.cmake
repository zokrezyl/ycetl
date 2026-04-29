# CMake generated Testfile for 
# Source directory: /home/misi/work/my/ycetl-private/unit_test/containers
# Build directory: /home/misi/work/my/ycetl-private/unit_test/containers/build/gcc
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_dynamic_array "/home/misi/work/my/ycetl-private/unit_test/containers/build/gcc/test_dynamic_array")
set_tests_properties(test_dynamic_array PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;45;add_test;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;49;create_test_executable;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;0;")
add_test(test_dynamic_array_iterator "/home/misi/work/my/ycetl-private/unit_test/containers/build/gcc/test_dynamic_array_iterator")
set_tests_properties(test_dynamic_array_iterator PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;45;add_test;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;50;create_test_executable;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;0;")
add_test(test_nested_dynamic_array "/home/misi/work/my/ycetl-private/unit_test/containers/build/gcc/test_nested_dynamic_array")
set_tests_properties(test_nested_dynamic_array PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;45;add_test;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;51;create_test_executable;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;0;")
add_test(test_dynamic_array_static_memory "/home/misi/work/my/ycetl-private/unit_test/containers/build/gcc/test_dynamic_array_static_memory")
set_tests_properties(test_dynamic_array_static_memory PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;45;add_test;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;52;create_test_executable;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;0;")
add_test(test_container "/home/misi/work/my/ycetl-private/unit_test/containers/build/gcc/test_container")
set_tests_properties(test_container PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;45;add_test;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;53;create_test_executable;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;0;")
subdirs("_deps/spdlog-build")
subdirs("_deps/ut-build")
subdirs("_deps/ytrace-private-build")
