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

set(ROCKSDB_COMMON_CMAKE_ARGS
    "-DCMAKE_SYSTEM_PREFIX_PATH=${EXTERNAL_BINARY_INSTALL_DIR}"
    "-DCMAKE_INSTALL_PREFIX=${EXTERNAL_BINARY_INSTALL_DIR}"
    "-DWITH_TESTS=OFF"
    "-DWITH_TOOLS=OFF"
)

if(ANDROID)
  set(ROCKSDB_CMAKE_ARGS 
    "${ROCKSDB_COMMON_CMAKE_ARGS}"
    "-DROCKSDB_LITE=ON"
    "-DPORTABLE=ON"
    "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
    "-DANDROID_PLATFORM=${ANDROID_PLATFORM}"
    "-DANDROID_STL=${ANDROID_STL}"
    "-DANDROID_ABI=${ANDROID_ABI}"
  )
elseif(IOS)
  set(ROCKSDB_CMAKE_ARGS 
    "${ROCKSDB_COMMON_CMAKE_ARGS}"
    "-DROCKSDB_LITE=ON"
    "-DPORTABLE=ON"
    "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
    "-DPLATFORM=${PLATFORM}"
    "-DSIMULATOR=${SIMULATOR}"
  )
elseif(WIN32)
  set(ROCKSDB_CMAKE_ARGS 
    "${ROCKSDB_COMMON_CMAKE_ARGS}"
    "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
    "-DROCKSDB_INSTALL_ON_WINDOWS=ON"
  )
else()
  set(ROCKSDB_CMAKE_ARGS "${ROCKSDB_COMMON_CMAKE_ARGS}")
endif()

if(BUILD_SHARED_LIBS)
  set(ROCKSDB_CMAKE_ARGS "${ROCKSDB_CMAKE_ARGS} -DCMAKE_POSITION_INDEPENDENT_CODE=ON")
endif()

# Download and unpack RocksDB at configure time
configure_file(CMakeLists.txt.rocksdb.in rocksdb-download/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rocksdb-download)
if(result)
  message(FATAL_ERROR "CMake step for rocksdb failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE}
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rocksdb-download)
if(result)
  message(FATAL_ERROR "Build step for rocksdb failed: ${result}")
endif()

# Provide the dir to the leveldb cmake configuration files
set(EXTERNAL_rocksdb_DIR ${EXTERNAL_BINARY_INSTALL_DIR}/lib/cmake/rocksdb PARENT_SCOPE)
set(EXTERNAL_rocksdb_INCLUDE_DIR ${EXTERNAL_BINARY_INSTALL_DIR}/include PARENT_SCOPE)
