# Copyright (C) 2019-2021 HERE Europe B.V.
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
  include(CheckCXXSourceCompiles)

  # run the cxx compile test with c++xx flags and pthreads library
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_CXX${CMAKE_CXX_STANDARD}_EXTENSION_COMPILE_OPTION}")
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

  set(HAVE_PTHREAD_SETNAME_NP OFF PARENT_SCOPE)
  check_cxx_source_compiles(
    "#define _GNU_SOURCE 1
    #include <pthread.h>
    int main() { pthread_t thread; pthread_setname_np(thread, NULL); }"
    HAVE_PTHREAD_SETNAME_NP)
endfunction(porting_do_checks)

