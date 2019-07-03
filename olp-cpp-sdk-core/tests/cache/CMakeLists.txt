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

file(GLOB SOURCES "unittest/*.cpp")

set(TARGET cache)

if (ANDROID OR IOS)
    add_library(${TARGET}-test ${SOURCES})
else()
    add_executable(${TARGET}-test ${SOURCES} main.cpp)
endif()

target_include_directories(${TARGET}-test
   PRIVATE ${PROJECT_SOURCE_DIR}/src/cache
)

target_link_libraries(${TARGET}-test
   olp-cpp-sdk-core
   custom-params
)

if (ANDROID)
    # Create Android test runner app
    target_link_libraries(${TARGET}-test gtest gmock)

    include(${CMAKE_SOURCE_DIR}/cmake/android/gen_android_test.cmake)
    gen_android_test_runner(${TARGET} ${TARGET}-test)

    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/android ${CMAKE_CURRENT_BINARY_DIR}/android)
elseif (IOS)
    # Create iOS test runner app
    target_link_libraries(${TARGET}-test gtest gmock)

    include(${CMAKE_SOURCE_DIR}/cmake/ios/gen_ios_test.cmake)
    gen_ios_test_runner(${TARGET} ${TARGET}-test)

    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/ios ${CMAKE_CURRENT_BINARY_DIR}/ios)
else()
    target_link_libraries(${TARGET}-test gtest_main gmock_main)
endif()

