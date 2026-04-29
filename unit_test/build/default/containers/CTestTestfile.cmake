# CMake generated Testfile for 
# Source directory: /home/misi/work/my/ycetl-private/unit_test/containers
# Build directory: /home/misi/work/my/ycetl-private/unit_test/build/default/containers
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_basic_string "/home/misi/work/my/ycetl-private/unit_test/build/default/containers/test_basic_string")
set_tests_properties(test_basic_string PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;57;add_test;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;0;")
add_test(test_basic_string_return "/home/misi/work/my/ycetl-private/unit_test/build/default/containers/test_basic_string_return")
set_tests_properties(test_basic_string_return PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;57;add_test;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;0;")
add_test(test_basic_string_with_memory "/home/misi/work/my/ycetl-private/unit_test/build/default/containers/test_basic_string_with_memory")
set_tests_properties(test_basic_string_with_memory PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;57;add_test;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;0;")
add_test(test_basic_string_with_static_memory "/home/misi/work/my/ycetl-private/unit_test/build/default/containers/test_basic_string_with_static_memory")
set_tests_properties(test_basic_string_with_static_memory PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;57;add_test;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;0;")
add_test(test_dynamic_array "/home/misi/work/my/ycetl-private/unit_test/build/default/containers/test_dynamic_array")
set_tests_properties(test_dynamic_array PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;57;add_test;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;0;")
add_test(test_set "/home/misi/work/my/ycetl-private/unit_test/build/default/containers/test_set")
set_tests_properties(test_set PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;57;add_test;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;0;")
add_test(test_vector "/home/misi/work/my/ycetl-private/unit_test/build/default/containers/test_vector")
set_tests_properties(test_vector PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;57;add_test;/home/misi/work/my/ycetl-private/unit_test/containers/CMakeLists.txt;0;")
subdirs("../_deps/spdlog-build")
subdirs("../_deps/ut-build")
subdirs("../_deps/ytrace-private-build")
