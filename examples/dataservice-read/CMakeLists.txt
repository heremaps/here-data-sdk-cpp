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
cmake_minimum_required(VERSION 3.5)

set(EDGE_SDK_DATASERVICE_READ_EXAMPLE_TARGET dataservice-read-example)

set(EDGE_SDK_EXAMPLE_SUCCESS_STRING "Reading the partition data from the specified catalog completed successfully")
set(EDGE_SDK_EXAMPLE_FAILURE_STRING "Failed to read data from the specified catalog!")

if (IOS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../helpers/cmake/gen_ios_example.cmake.in)

    gen_ios_example_application(${EDGE_SDK_DATASERVICE_READ_EXAMPLE_TARGET}
                                ${EDGE_SDK_EXAMPLE_SUCCESS_STRING}
                                ${EDGE_SDK_EXAMPLE_FAILURE_STRING})

elseif (ANDROID)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../helpers/cmake/gen_android_example.cmake.in)

    set(EDGE_SDK_EXAMPLE_PACKAGE_NAME "dataservice_read_example")
    set(EDGE_SDK_EXAMPLE_PACKAGE_JNI_NAME "dataservice_1read_1example")
    gen_android_example_application(${EDGE_SDK_DATASERVICE_READ_EXAMPLE_TARGET}
                                    ${EDGE_SDK_EXAMPLE_PACKAGE_NAME}
                                    ${EDGE_SDK_EXAMPLE_PACKAGE_JNI_NAME}
                                    ${EDGE_SDK_EXAMPLE_SUCCESS_STRING}
                                    ${EDGE_SDK_EXAMPLE_FAILURE_STRING})
else()
    project(${EDGE_SDK_DATASERVICE_READ_EXAMPLE_TARGET})

    find_package(olp-cpp-sdk-authentication REQUIRED)
    find_package(olp-cpp-sdk-dataservice-read REQUIRED)

    add_executable(${EDGE_SDK_DATASERVICE_READ_EXAMPLE_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/example.h ${CMAKE_CURRENT_SOURCE_DIR}/example.cpp)

    target_sources(${EDGE_SDK_DATASERVICE_READ_EXAMPLE_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

    target_link_libraries(${EDGE_SDK_DATASERVICE_READ_EXAMPLE_TARGET}
        olp-cpp-sdk-authentication
        olp-cpp-sdk-dataservice-read
    )
endif()

