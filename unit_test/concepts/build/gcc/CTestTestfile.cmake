# CMake generated Testfile for 
# Source directory: /home/misi/work/my/ycetl-private/unit_test/concepts
# Build directory: /home/misi/work/my/ycetl-private/unit_test/concepts/build/gcc
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_simple "/home/misi/work/my/ycetl-private/unit_test/concepts/build/gcc/test_simple")
set_tests_properties(test_simple PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/concepts/CMakeLists.txt;56;add_test;/home/misi/work/my/ycetl-private/unit_test/concepts/CMakeLists.txt;0;")
add_test(test_template_info "/home/misi/work/my/ycetl-private/unit_test/concepts/build/gcc/test_template_info")
set_tests_properties(test_template_info PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/concepts/CMakeLists.txt;56;add_test;/home/misi/work/my/ycetl-private/unit_test/concepts/CMakeLists.txt;0;")
subdirs("_deps/spdlog-build")
subdirs("_deps/ut-build")
subdirs("_deps/ytrace-private-build")
