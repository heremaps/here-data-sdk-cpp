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

if(NOT OLP_SDK_BUILD_DOC)
    message(STATUS "Build SDK documentation disabled")
    return()
endif()

if (${CMAKE_VERSION} VERSION_LESS "3.9")
    # Old versions of CMake have really poor support for Doxygen generation.
    message(STATUS "Doxygen generation only enabled for cmake 3.9 and higher")
    return()
endif()

find_package(Doxygen)
if(NOT DOXYGEN_FOUND)
    message(STATUS "Doxygen is needed to build the documentation.")
    return()
endif()

set(COMPONENT_DIRS
    olp-cpp-sdk-authentication/include
    olp-cpp-sdk-core/include
    olp-cpp-sdk-dataservice-read/include
    olp-cpp-sdk-dataservice-write/include
)
set(MAINPAGE_FILE README.md)
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${MAINPAGE_FILE})
set(DOXYGEN_ENABLE_PREPROCESSING YES)
set(DOXYGEN_MACRO_EXPANSION YES)
set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
set(DOXYGEN_PREDEFINED OLP_SDK_ENABLE_DEFAULT_CACHE)
doxygen_add_docs(docs
    ${COMPONENT_DIRS} ${MAINPAGE_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    COMMENT "Generate HTML documentation")
