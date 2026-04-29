# CMake generated Testfile for 
# Source directory: /home/misi/work/my/ycetl-private/test
# Build directory: /home/misi/work/my/ycetl-private/test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_allocator "/home/misi/work/my/ycetl-private/test/test_allocator")
set_tests_properties(test_allocator PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/test/CMakeLists.txt;43;add_test;/home/misi/work/my/ycetl-private/test/CMakeLists.txt;0;")
add_test(test_boost_ut "/home/misi/work/my/ycetl-private/test/test_boost_ut")
set_tests_properties(test_boost_ut PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/test/CMakeLists.txt;43;add_test;/home/misi/work/my/ycetl-private/test/CMakeLists.txt;0;")
add_test(test_dynamic_arena "/home/misi/work/my/ycetl-private/test/test_dynamic_arena")
set_tests_properties(test_dynamic_arena PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/test/CMakeLists.txt;43;add_test;/home/misi/work/my/ycetl-private/test/CMakeLists.txt;0;")
add_test(test_dynamic_arena_manual "/home/misi/work/my/ycetl-private/test/test_dynamic_arena_manual")
set_tests_properties(test_dynamic_arena_manual PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/test/CMakeLists.txt;43;add_test;/home/misi/work/my/ycetl-private/test/CMakeLists.txt;0;")
add_test(test_m_storage "/home/misi/work/my/ycetl-private/test/test_m_storage")
set_tests_properties(test_m_storage PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/test/CMakeLists.txt;43;add_test;/home/misi/work/my/ycetl-private/test/CMakeLists.txt;0;")
add_test(test_memory "/home/misi/work/my/ycetl-private/test/test_memory")
set_tests_properties(test_memory PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/test/CMakeLists.txt;43;add_test;/home/misi/work/my/ycetl-private/test/CMakeLists.txt;0;")
add_test(test_object_pool "/home/misi/work/my/ycetl-private/test/test_object_pool")
set_tests_properties(test_object_pool PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/test/CMakeLists.txt;43;add_test;/home/misi/work/my/ycetl-private/test/CMakeLists.txt;0;")
add_test(test_rom_arena "/home/misi/work/my/ycetl-private/test/test_rom_arena")
set_tests_properties(test_rom_arena PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/test/CMakeLists.txt;43;add_test;/home/misi/work/my/ycetl-private/test/CMakeLists.txt;0;")
add_test(test_trace "/home/misi/work/my/ycetl-private/test/test_trace")
set_tests_properties(test_trace PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/test/CMakeLists.txt;43;add_test;/home/misi/work/my/ycetl-private/test/CMakeLists.txt;0;")
add_test(test_trivial_array "/home/misi/work/my/ycetl-private/test/test_trivial_array")
set_tests_properties(test_trivial_array PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/test/CMakeLists.txt;43;add_test;/home/misi/work/my/ycetl-private/test/CMakeLists.txt;0;")
add_test(test_ut_vector "/home/misi/work/my/ycetl-private/test/test_ut_vector")
set_tests_properties(test_ut_vector PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/test/CMakeLists.txt;43;add_test;/home/misi/work/my/ycetl-private/test/CMakeLists.txt;0;")
add_test(test_vector "/home/misi/work/my/ycetl-private/test/test_vector")
set_tests_properties(test_vector PROPERTIES  _BACKTRACE_TRIPLES "/home/misi/work/my/ycetl-private/test/CMakeLists.txt;43;add_test;/home/misi/work/my/ycetl-private/test/CMakeLists.txt;0;")
subdirs("_deps/spdlog-build")
subdirs("_deps/ut-build")
subdirs("_deps/ytrace-private-build")
