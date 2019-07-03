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

# Boost
# Download and unpack boost at configure time
set(BOOST_CMAKE_ARGS)
if(ANDROID)
  set(BOOST_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DANDROID_PLATFORM=${ANDROID_PLATFORM} -DANDROID_STL=${ANDROID_STL} -DANDROID_ABI=${ANDROID_ABI}")
elseif(IOS)
  set(BOOST_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DPLATFORM=${PLATFORM} -DSIMULATOR=${SIMULATOR} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif()

configure_file(CMakeLists.txt.boost.in download/CMakeLists.txt)

set(CMAKE_VERBOSE_MAKEFILE ON)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download)
if(result)
  message(FATAL_ERROR "CMake step for boost failed: ${result}")
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-Boost.txt
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/config/src)
file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/config/src/CMakeLists-Boost.txt
     ${CMAKE_CURRENT_BINARY_DIR}/config/src/CMakeLists.txt)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/config/src/include)

execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download)
if(result)
  message(FATAL_ERROR "Build step for boost failed: ${result}")
endif()

#Add boost directly to our build.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/src
                 ${CMAKE_CURRENT_BINARY_DIR}/build
                 EXCLUDE_FROM_ALL)


# Provide the dir to the boost cmake configuration files
set(EXTERNAL_BOOST_ROOT ${CMAKE_CURRENT_BINARY_DIR}/build PARENT_SCOPE)
set(EXTERNAL_BOOST_ROOT_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/src PARENT_SCOPE)
if (IOS)
    set(EXTERNAL_BOOST_ROOT_LIB ${CMAKE_CURRENT_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}-${XCODE_IOS_PLATFORM} PARENT_SCOPE)
else()
    set(EXTERNAL_BOOST_ROOT_LIB ${CMAKE_CURRENT_BINARY_DIR}/build PARENT_SCOPE)
endif()
