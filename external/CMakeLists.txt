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

# Do not export external projects to the cmake user package registry
set(CMAKE_EXPORT_NO_PACKAGE_REGISTRY ON)

# External config dirs
set(EXTERNAL_BINARY_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install)

include(ProcessorCount)
ProcessorCount(N)

# get CMAKE_MAKE_PROGRAM to know when to change build flags for Windows command line build
get_filename_component(compilername ${CMAKE_MAKE_PROGRAM} NAME)

if(NOT N EQUAL 0)
    if (${compilername} STREQUAL "MSBuild.exe")
        set(EXTERNAL_BUILD_FLAGS -- /m:${N})
    elseif(NOT (CMAKE_GENERATOR STREQUAL Xcode))
        set(EXTERNAL_BUILD_FLAGS -- -j${N})
    endif()
endif()

# Setup versions for external project 
set(OLP_SDK_CPP_GOOGLETEST_URL "https://github.com/google/googletest.git")
set(OLP_SDK_CPP_GOOGLETEST_TAG "release-1.10.0")

set(OLP_SDK_CPP_SNAPPY_URL "https://github.com/google/snappy.git")
set(OLP_SDK_CPP_SNAPPY_TAG "1.1.7")

set(OLP_SDK_CPP_LEVELDB_URL "https://github.com/google/leveldb.git")
set(OLP_SDK_CPP_LEVELDB_TAG "1.21")

set(OLP_SDK_CPP_RAPIDJSON_URL "https://github.com/Tencent/rapidjson.git")
set(OLP_SDK_CPP_RAPIDJSON_TAG "master")

set(OLP_SDK_CPP_BOOST_URL "https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.gz")

# Add external projects
add_subdirectory(googletest)

add_subdirectory(rapidjson)
set(RapidJSON_DIR ${EXTERNAL_RapidJSON_DIR} PARENT_SCOPE)

add_subdirectory(leveldb)
set(leveldb_DIR ${EXTERNAL_leveldb_DIR} PARENT_SCOPE)
set(leveldb_INCLUDE_DIR ${EXTERNAL_leveldb_INCLUDE_DIR} PARENT_SCOPE)
set(Snappy_DIR ${EXTERNAL_Snappy_DIR} PARENT_SCOPE)
if(ANDROID OR IOS OR WIN32)
    add_subdirectory(boost)
    set(BOOST_ROOT ${EXTERNAL_BOOST_ROOT} PARENT_SCOPE)
    set(Boost_INCLUDE_DIR ${EXTERNAL_BOOST_ROOT_INCLUDE} PARENT_SCOPE)
    set(Boost_LIBRARY_DIR ${EXTERNAL_BOOST_ROOT_LIB} PARENT_SCOPE)
endif()
