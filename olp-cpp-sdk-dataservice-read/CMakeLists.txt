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

project(olp-cpp-sdk-dataservice-read VERSION 1.2.0)
set(DESCRIPTION "C++ API library for reading OLP data")

file(GLOB_RECURSE INC "include/*.h*")
file(GLOB_RECURSE SRC "src/*.*")

add_library(${PROJECT_NAME}
    ${SRC}
    ${INC})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    PRIVATE ${olp-cpp-sdk-core_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        olp-cpp-sdk-core
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE DATASERVICE_READ_LIBRARY)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC DATASERVICE_READ_SHARED_LIBRARY)
endif()

# install component
file(GLOB API_HEADERS "include/olp/dataservice/read/*.h")
file(GLOB MODEL_HEADERS "include/olp/dataservice/read/model/*.h")

install (FILES ${API_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/dataservice/read)
install (FILES ${MODEL_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/dataservice/read/model)

export_config()

if(OLP_SDK_ENABLE_TESTING)
    add_subdirectory(tests)
endif()

