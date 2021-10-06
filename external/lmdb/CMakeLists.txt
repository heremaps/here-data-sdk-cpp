# Copyright (C) 2021 HERE Europe B.V.
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

# Download and unpack lmdb at configure time
configure_file(CMakeLists.txt.lmdb.in download/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . ${COMMON_GENERATE_FLAGS}
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download)
if(result)
  message(FATAL_ERROR "CMake step for lmdb failed: ${result}")
endif()

# Copy CMakeLists
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-lmdb.txt
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/config/lmdb)
file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/config/lmdb/CMakeLists-lmdb.txt
     ${CMAKE_CURRENT_BINARY_DIR}/config/lmdb/CMakeLists.txt)

# Copy lmdbConfig.cmake
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lmdbConfig.cmake.in
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/config/lmdb/cmake)
file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/config/lmdb/cmake/lmdbConfig.cmake.in
     ${CMAKE_CURRENT_BINARY_DIR}/config/lmdb/cmake/lmdbConfig.cmake)

execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE}
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download)
if(result)
  message(FATAL_ERROR "Build step for lmdb failed: ${result}")
endif()

# Provide the dir to the lmdb cmake configuration files
set(EXTERNAL_lmdb_DIR ${EXTERNAL_BINARY_INSTALL_DIR}/lib/cmake/lmdb PARENT_SCOPE)
set(EXTERNAL_lmdb_INCLUDE_DIR ${EXTERNAL_BINARY_INSTALL_DIR}/include PARENT_SCOPE)
