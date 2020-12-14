# Copyright (C) 2019-2020 HERE Europe B.V.
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

project(olp-cpp-sdk-authentication VERSION 1.10.0)
set(DESCRIPTION "C++ API library for accesing HERE Account authentication service")

file(GLOB_RECURSE AUTHENTICATION_INC "include/*.h*")
file(GLOB_RECURSE AUTHENTICATION_SRC "src/*.*")

add_library(${PROJECT_NAME}
    ${AUTHENTICATION_SRC}
    ${AUTHENTICATION_INC})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# Used also in the package config file
set(PROJECT_LIBS olp-cpp-sdk-core)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${PROJECT_LIBS}
)

# On MINGW boost uses bcrypt library
if (MINGW)
    target_link_libraries(${PROJECT_NAME} PRIVATE bcrypt )
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE AUTHENTICATION_LIBRARY)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC AUTHENTICATION_SHARED_LIBRARY)
endif()

# install component
install (FILES ${AUTHENTICATION_INC} DESTINATION ${INCLUDE_DIRECTORY}/olp/authentication)

export_config()

if(OLP_SDK_ENABLE_TESTING)
    add_subdirectory(tests)
endif()
