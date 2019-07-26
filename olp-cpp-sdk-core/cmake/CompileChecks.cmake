# Copyright (C) 2019 HERE Europe B.V.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
# License-Filename: LICENSE

function(porting_do_checks)
  if (ARGC GREATER 0)
    set(PATH_TO_INPUT_FILE ${ARGV0}/)
    list(REMOVE_AT ARGN 0)
  endif()

  include(CheckCXXSourceCompiles)
  include(TestBigEndian)

  test_big_endian(PORTING_SYSTEM_BIG_ENDIAN)

  # run the cxx compile test with c++xx flags and pthreads library
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_CXX${CMAKE_CXX_STANDARD}_EXTENSION_COMPILE_OPTION}")
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

  set(HAVE_STD_OPTIONAL OFF PARENT_SCOPE)
  check_cxx_source_compiles(
    "#include <optional>
    int main() { return *std::make_optional(4); }"
    HAVE_STD_OPTIONAL)

  set(HAVE_EXPERIMENTAL_OPTIONAL OFF PARENT_SCOPE)
  check_cxx_source_compiles(
    "#include <experimental/optional>
    int main() { return *std::experimental::make_optional(4); }"
    HAVE_EXPERIMENTAL_OPTIONAL)

  set(HAVE_STD_FILESYSTEM OFF PARENT_SCOPE)
  check_cxx_source_compiles(
    "#include <filesystem>
    int main() { return static_cast<int>(std::filesystem::file_type::directory); }"
    HAVE_STD_FILESYSTEM)

  set(HAVE_PTHREAD_SETNAME_NP OFF PARENT_SCOPE)
  check_cxx_source_compiles(
    "#define _GNU_SOURCE 1
    #include <pthread.h>
    int main() { pthread_t thread; pthread_setname_np(thread, NULL); }"
    HAVE_PTHREAD_SETNAME_NP)

  configure_file(${PATH_TO_INPUT_FILE}porting_config.h.in
    include/olp/core/porting/porting_config.h
    @ONLY)
endfunction(porting_do_checks)

