# Copyright (C) 2019-2026 HERE Europe B.V.
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

set(OLP_SDK_TESTS_COMMON_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/matchers/NetworkUrlMatchers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/CacheMock.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/NetworkMock.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/TaskSchedulerMock.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ApiDefaultResponses.h
    ${CMAKE_CURRENT_SOURCE_DIR}/KeyValueCacheTestable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlatformUrlsGenerator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ReadDefaultResponses.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ResponseGenerator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/WriteDefaultResponses.h
)

set(OLP_SDK_TESTS_COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/CacheMock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/NetworkMock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ApiDefaultResponses.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PlatformUrlsGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ReadDefaultResponses.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ResponseGenerator.cpp
)

add_library(olp-cpp-sdk-tests-common
        ${OLP_SDK_TESTS_COMMON_SOURCES}
        ${OLP_SDK_TESTS_COMMON_HEADERS})

target_include_directories(olp-cpp-sdk-tests-common PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)

target_include_directories(olp-cpp-sdk-tests-common 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/../../olp-cpp-sdk-dataservice-read/src/
)

target_compile_definitions(olp-cpp-sdk-tests-common PUBLIC BOOST_CONTAINER_NO_LIB)
target_compile_definitions(olp-cpp-sdk-tests-common PUBLIC BOOST_JSON_NO_LIB)

target_link_libraries(olp-cpp-sdk-tests-common
    PUBLIC
        gmock
        olp-cpp-sdk-core
        olp-cpp-sdk-dataservice-write
        olp-cpp-sdk-dataservice-read
)
