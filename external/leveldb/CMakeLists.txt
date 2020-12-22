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

# Download and unpack leveldb at configure time
configure_file(CMakeLists.txt.leveldb.in download/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . ${COMMON_GENERATE_FLAGS}
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download)
if(result)
  message(FATAL_ERROR "CMake step for leveldb failed: ${result}")
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-leveldb.txt
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/config/leveldb)
file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/config/leveldb/CMakeLists-leveldb.txt
     ${CMAKE_CURRENT_BINARY_DIR}/config/leveldb/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE}
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download)
if(result)
  message(FATAL_ERROR "Build step for leveldb failed: ${result}")
endif()

# Provide the dir to the leveldb cmake configuration files
set(EXTERNAL_leveldb_DIR ${EXTERNAL_BINARY_INSTALL_DIR}/lib/cmake/leveldb PARENT_SCOPE)
set(EXTERNAL_Snappy_DIR ${EXTERNAL_BINARY_INSTALL_DIR}/lib/cmake/Snappy PARENT_SCOPE)
set(EXTERNAL_leveldb_INCLUDE_DIR ${EXTERNAL_BINARY_INSTALL_DIR}/include PARENT_SCOPE)

