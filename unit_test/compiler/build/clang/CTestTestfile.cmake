# CMake generated Testfile for 
# Source directory: /home/misi/work/my/ycetl-private/unit_test/compiler
# Build directory: /home/misi/work/my/ycetl-private/unit_test/compiler/build/clang
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_1 "/home/misi/work/my/ycetl-private/unit_test/compiler/build/clang/test_1")
set_tests_properties(test_1 PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/unit_test/compiler/CMakeLists.txt;33;add_test;/home/misi/work/my/ycetl-private/unit_test/compiler/CMakeLists.txt;0;")
subdirs("_deps/ut-build")
subdirs("_deps/ytrace-private-build")
