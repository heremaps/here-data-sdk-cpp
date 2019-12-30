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

set(COMMON_PLATFORM_FLAGS)

if(ANDROID)
  set(COMMON_PLATFORM_FLAGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} 
                            -DANDROID_PLATFORM=${ANDROID_PLATFORM}
                            -DANDROID_STL=${ANDROID_STL}
                            -DANDROID_ABI=${ANDROID_ABI})
elseif(IOS)
  set(COMMON_PLATFORM_FLAGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                            -DPLATFORM=${PLATFORM}
                            -DSIMULATOR=${SIMULATOR})
endif()

if(BUILD_SHARED_LIBS)
  set(COMMON_PLATFORM_FLAGS ${COMMON_PLATFORM_FLAGS} -DCMAKE_POSITION_INDEPENDENT_CODE=ON)
endif()

# Download and unpack leveldb at configure time
configure_file(CMakeLists.txt.leveldb.in download/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download)
if(result)
  message(FATAL_ERROR "CMake step for leveldb failed: ${result}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE}
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download)
if(result)
  message(FATAL_ERROR "Build step for leveldb failed: ${result}")
endif()
