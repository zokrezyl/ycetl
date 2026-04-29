# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/misi/work/my/ycetl-private/unit_test/build/default/_deps/ytrace-private-src")
  file(MAKE_DIRECTORY "/home/misi/work/my/ycetl-private/unit_test/build/default/_deps/ytrace-private-src")
endif()
file(MAKE_DIRECTORY
  "/home/misi/work/my/ycetl-private/unit_test/build/default/_deps/ytrace-private-build"
  "/home/misi/work/my/ycetl-private/unit_test/build/default/_deps/ytrace-private-subbuild/ytrace-private-populate-prefix"
  "/home/misi/work/my/ycetl-private/unit_test/build/default/_deps/ytrace-private-subbuild/ytrace-private-populate-prefix/tmp"
  "/home/misi/work/my/ycetl-private/unit_test/build/default/_deps/ytrace-private-subbuild/ytrace-private-populate-prefix/src/ytrace-private-populate-stamp"
  "/home/misi/work/my/ycetl-private/unit_test/build/default/_deps/ytrace-private-subbuild/ytrace-private-populate-prefix/src"
  "/home/misi/work/my/ycetl-private/unit_test/build/default/_deps/ytrace-private-subbuild/ytrace-private-populate-prefix/src/ytrace-private-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/misi/work/my/ycetl-private/unit_test/build/default/_deps/ytrace-private-subbuild/ytrace-private-populate-prefix/src/ytrace-private-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/misi/work/my/ycetl-private/unit_test/build/default/_deps/ytrace-private-subbuild/ytrace-private-populate-prefix/src/ytrace-private-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
