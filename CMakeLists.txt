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

# Build the sdk targets
project(olp-cpp-sdk VERSION 0.6.0)

# Add preprocessor definitions for the SDK version and platform name
add_definitions(-DEDGE_SDK_VERSION_STRING=\"${olp-cpp-sdk_VERSION}\")
add_definitions(-DEDGE_SDK_VERSION_MAJOR=${olp-cpp-sdk_VERSION_MAJOR})
add_definitions(-DEDGE_SDK_VERSION_MINOR=${olp-cpp-sdk_VERSION_MINOR})
add_definitions(-DEDGE_SDK_VERSION_PATCH=${olp-cpp-sdk_VERSION_PATCH})

if (APPLE AND IOS)
    # To distinguish iOS platform from other Darwin platforms
    add_definitions(-DEDGE_SDK_PLATFORM_NAME=\"iOS\")
else()
    add_definitions(-DEDGE_SDK_PLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\")
endif()

# Options
option(EDGE_SDK_ENABLE_TESTING "Flag to enable/disable building unit and integration tests" ON)
option(EDGE_SDK_BUILD_DOC "Build SDK documentation" OFF)
option(EDGE_SDK_NO_EXCEPTION "Turn off EXCEPTION" OFF)
option(EDGE_SDK_BUILD_EXTERNAL_DEPS "Download and build external dependencies" ON)
option(EDGE_SDK_BUILD_EXAMPLES "Enable examples targets" OFF)
option(EDGE_SDK_MSVC_PARALLEL_BUILD_ENABLE "Enable parallel build on MSVC" ON)

# C++ standard version. Minimum supported version is 11.
set(CMAKE_CXX_STANDARD 11)

# Set no exception
if (EDGE_SDK_NO_EXCEPTION)
    set(CMAKE_CXX_FLAGS "-fno-exceptions ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "-fno-exceptions ${CMAKE_C_FLAGS}")
endif()

if (MSVC AND EDGE_SDK_MSVC_PARALLEL_BUILD_ENABLE)
    add_compile_options(/MP)
    message(STATUS "MSVC - Using parallel compilation (/MP)")
endif (MSVC AND EDGE_SDK_MSVC_PARALLEL_BUILD_ENABLE)

# Add cmake dir to search list
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Save root sdk folder path
get_filename_component(OLP_CPP_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# Include common scripts
include(common)

# Include code coverage variables
include(coverage)

if(EDGE_SDK_BUILD_EXTERNAL_DEPS)
    # Add third-party dependencies
    add_subdirectory(external)
endif()

# Include testutils
add_subdirectory(testutils/custom-params)

# Add the sdks
add_sdks()

# Add docs
add_subdirectory(docs)

# Add example
if(EDGE_SDK_BUILD_EXAMPLES)
    add_subdirectory(examples/dataservice-read)
    add_subdirectory(examples/dataservice-write)
endif()

# Add uninstall script
add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_MODULE_PATH}/uninstall.cmake")

