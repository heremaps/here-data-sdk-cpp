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

if(NOT EDGE_SDK_ENABLE_TESTING)
    return()
endif()

project(custom-params)

set(TARGET_NAME ${PROJECT_NAME})

set(${TARGET_NAME}_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CustomParameters.cpp"
)
set(${TARGET_NAME}_INC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/testutils/CustomParameters.hpp"
)

add_library(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE
    ${${TARGET_NAME}_SRC}
    ${${TARGET_NAME}_INC}
)

target_include_directories(${TARGET_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if (ANDROID OR IOS)
    target_link_libraries(${TARGET_NAME} gtest gmock)
else()
    target_link_libraries(${TARGET_NAME} gtest_main gmock_main)
endif()

