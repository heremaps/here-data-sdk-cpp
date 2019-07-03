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

# RapidJSON
# Download and unpack rapidjson at configure time
configure_file(CMakeLists.txt.rapidjson.in download/CMakeLists.txt)

# Workaround to override options in rapidjson cmake
option(RAPIDJSON_BUILD_DOC "Build rapidjson documentation." OFF)
option(RAPIDJSON_BUILD_EXAMPLES "Build rapidjson examples." OFF)
option(RAPIDJSON_BUILD_TESTS "Build rapidjson perftests and unittests." OFF)
option(RAPIDJSON_BUILD_THIRDPARTY_GTEST
    "Use gtest installation in `thirdparty/gtest` by default if available" OFF)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download)
if(result)
  message(FATAL_ERROR "CMake step for rapidjson failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download)
if(result)
  message(FATAL_ERROR "Build step for rapidjson failed: ${result}")
endif()

# Add rapidjson directly to our build.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/src
                 ${CMAKE_CURRENT_BINARY_DIR}/build
                 EXCLUDE_FROM_ALL)

# Provide the dir to the rapidjson cmake configuration files
set(EXTERNAL_RapidJSON_DIR ${CMAKE_CURRENT_BINARY_DIR}/build PARENT_SCOPE)

