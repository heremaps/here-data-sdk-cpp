# Copyright (C) 2019-2020 HERE Europe B.V.
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

# Boost
# Download and unpack boost at configure time

if (WIN32)
    set(BOOTSTRAP_CMD bootstrap.bat)
    set(B2_CMD b2.exe)
else()
    # Reset the CXX (path to compiler) before running bootstrap since we can cross compile.
    # Else we need to specify the -sysroot argument with CXXFLAGS.
    set(BOOTSTRAP_CMD unset CXX && ./bootstrap.sh)
    set(B2_CMD ./b2)
endif()

configure_file(CMakeLists.txt.boost.in download/CMakeLists.txt @ONLY)

set(CMAKE_VERBOSE_MAKEFILE ON)

execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download)
if(result)
  message(FATAL_ERROR "CMake step for boost failed: ${result}")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download)
if(result)
  message(FATAL_ERROR "Build step for boost failed: ${result}")
endif()

# We are using boost header only, we don't care about libs
set(EXTERNAL_BOOST_ROOT ${CMAKE_CURRENT_BINARY_DIR}/external_boost PARENT_SCOPE)
set(EXTERNAL_BOOST_ROOT_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/external_boost PARENT_SCOPE)
set(EXTERNAL_BOOST_ROOT_LIB dummy_path PARENT_SCOPE)
