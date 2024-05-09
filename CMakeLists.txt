# Copyright (C) 2019-2024 HERE Europe B.V.
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

cmake_minimum_required(VERSION 3.9)

# Build the sdk targets
project(olp-cpp-sdk VERSION 1.19.0)

# Add preprocessor definitions for the SDK version and platform name
add_definitions(-DOLP_SDK_VERSION_STRING=\"${olp-cpp-sdk_VERSION}\")
add_definitions(-DOLP_SDK_VERSION_MAJOR=${olp-cpp-sdk_VERSION_MAJOR})
add_definitions(-DOLP_SDK_VERSION_MINOR=${olp-cpp-sdk_VERSION_MINOR})
add_definitions(-DOLP_SDK_VERSION_PATCH=${olp-cpp-sdk_VERSION_PATCH})

if (APPLE AND IOS)
    # To distinguish iOS platform from other Darwin platforms
    add_definitions(-DOLP_SDK_PLATFORM_NAME=\"iOS\")
else()
    add_definitions(-DOLP_SDK_PLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\")
endif()

# Options
option(OLP_SDK_ENABLE_TESTING "Flag to enable/disable building unit and integration tests" ON)
option(OLP_SDK_BUILD_DOC "Build SDK documentation" OFF)
option(OLP_SDK_NO_EXCEPTION "Disable exception handling" OFF)
option(OLP_SDK_BOOST_THROW_EXCEPTION_EXTERNAL "The boost::throw_exception() is defined externally" OFF)
option(OLP_SDK_BUILD_EXTERNAL_DEPS "Download and build external dependencies" ON)
option(OLP_SDK_BUILD_EXAMPLES "Enable examples targets" OFF)
option(OLP_SDK_MSVC_PARALLEL_BUILD_ENABLE "Enable parallel build on MSVC" ON)
option(OLP_SDK_DISABLE_DEBUG_LOGGING "Disable debug and trace level logging" OFF)
option(OLP_SDK_ENABLE_DEFAULT_CACHE "Enable default cache implementation based on LevelDB" ON)
option(OLP_SDK_ENABLE_DEFAULT_CACHE_LMDB "Enable default cache implementation based on LMDB" OFF)
option(OLP_SDK_ENABLE_ANDROID_CURL "Enable curl based network layer for Android" OFF)
option(OLP_SDK_ENABLE_IOS_BACKGROUND_DOWNLOAD "Enable iOS network layer downloading in background. Under testing." OFF)
option(OLP_SDK_ENABLE_OFFLINE_MODE "Enable offline mode. Network layer is excluded from the build and all network requests returned with error." OFF)

# C++ standard version. Minimum supported version is 11.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Set no exception
if (OLP_SDK_NO_EXCEPTION)
    set(CMAKE_CXX_FLAGS "-fno-exceptions ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "-fno-exceptions ${CMAKE_C_FLAGS}")
endif()

if (MSVC AND OLP_SDK_MSVC_PARALLEL_BUILD_ENABLE)
    add_compile_options(/MP)
    message(STATUS "MSVC - Using parallel compilation (/MP)")
endif (MSVC AND OLP_SDK_MSVC_PARALLEL_BUILD_ENABLE)

# Add cmake dir to search list
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Save root sdk folder path
get_filename_component(OLP_CPP_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# Include common scripts
include(common)

# Include code coverage variables
include(coverage)

if (OLP_SDK_BUILD_EXTERNAL_DEPS)
    # Add third-party dependencies
    set(EXTERNAL_BINARY_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install)
    add_subdirectory(external)
    list(APPEND CMAKE_PREFIX_PATH ${EXTERNAL_BINARY_INSTALL_DIR})
    list(APPEND CMAKE_FIND_ROOT_PATH "${EXTERNAL_BINARY_INSTALL_DIR}/")
endif()

# Add the sdks
add_sdks()

# Add docs
add_subdirectory(docs)

if(OLP_SDK_ENABLE_TESTING)
    add_subdirectory(tests/custom-params)
    # Add tests
    add_subdirectory(tests/common)
    add_subdirectory(tests/functional)
    add_subdirectory(tests/functional/network)
    add_subdirectory(tests/integration)
    add_subdirectory(tests/performance)
endif()

# Add example
if(OLP_SDK_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Add uninstall script
add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_MODULE_PATH}/uninstall.cmake")
