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
project(olp-cpp-sdk-dataservice-write VERSION 0.7.0)
set(DESCRIPTION "C++ API library for writing data to OLP")

file(GLOB_RECURSE DATASERVICE_WRITE_INC "include/*.h*")
file(GLOB_RECURSE DATASERVICE_WRITE_SRC "src/*.*")

add_library(${PROJECT_NAME}
    ${DATASERVICE_WRITE_INC}
    ${DATASERVICE_WRITE_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>)

# Used also in the package config file
set(PROJECT_LIBS olp-cpp-sdk-core)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${PROJECT_LIBS}
)

# install component
file(GLOB DATASERVICE_WRITE_API_HEADERS "include/olp/dataservice/write/*.h")
file(GLOB DATASERVICE_WRITE_GENERATED_MODEL_HEADERS "include/olp/dataservice/write/generated/model/*.h")
file(GLOB DATASERVICE_WRITE_MODEL_HEADERS "include/olp/dataservice/write/model/*.h")

install (FILES ${DATASERVICE_WRITE_API_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/dataservice/write)
install (FILES ${DATASERVICE_WRITE_GENERATED_MODEL_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/dataservice/write/generated/model)
install (FILES ${DATASERVICE_WRITE_MODEL_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/dataservice/write/model)

export_config()

if(EDGE_SDK_ENABLE_TESTING)
    add_subdirectory(test)
endif()
