# CMake generated Testfile for 
# Source directory: /home/misi/work/my/ycetl-private/examples/yce
# Build directory: /home/misi/work/my/ycetl-private/examples/yce/build/default
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(multitype_handler "/home/misi/work/my/ycetl-private/examples/yce/build/default/multitype_handler")
set_tests_properties(multitype_handler PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/examples/yce/CMakeLists.txt;45;add_test;/home/misi/work/my/ycetl-private/examples/yce/CMakeLists.txt;0;")
add_test(multitype_storage "/home/misi/work/my/ycetl-private/examples/yce/build/default/multitype_storage")
set_tests_properties(multitype_storage PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/examples/yce/CMakeLists.txt;45;add_test;/home/misi/work/my/ycetl-private/examples/yce/CMakeLists.txt;0;")
subdirs("_deps/spdlog-build")
subdirs("_deps/ut-build")
subdirs("_deps/ytrace-private-build")
