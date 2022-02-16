# Copyright (C) 2019-2021 HERE Europe B.V.
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
if(NOT ANDROID OR OLP_SDK_ENABLE_ANDROID_CURL)
    set(NETWORK_ANDROID_LIBRARIES)
    return()
endif()

add_definitions(-DOLP_SDK_NETWORK_HAS_ANDROID)
set(NETWORK_ANDROID_LIBRARIES ${ANDROID_LIBRARY})
include_directories(${ANDROID_INCLUDE_DIR})

include(UseJava)

# cmake wants to always use posix path separators due to the android ndk toolchain
# This does not work with nmake on Windows. Force WIN32
if(CMAKE_HOST_WIN32 AND NOT WIN32)
    set(FORCED_WIN32 TRUE)
    set(WIN32 TRUE)
endif()

set(CMAKE_JAVA_COMPILE_FLAGS -source 1.7 -target 1.7)
set(CMAKE_JAR_CLASSES_PREFIX com/here/olp/network)

include(${CMAKE_CURRENT_LIST_DIR}/GetAndroidVariables.cmake)
get_android_jar_path(CMAKE_JAVA_INCLUDE_PATH ANDROID_SDK_ROOT ANDROID_PLATFORM)

set(OLP_SDK_NETWORK_VERSION 0.0.1)
set(OLP_SDK_ANDROID_HTTP_CLIENT_JAR OlpHttpClient)

add_jar(${OLP_SDK_ANDROID_HTTP_CLIENT_JAR}
    SOURCES ${CMAKE_CURRENT_LIST_DIR}/../src/http/android/HttpClient.java
    VERSION ${OLP_SDK_NETWORK_VERSION}
)

# add_jar() doesn't add the symlink to the version automatically under Windows
if (WIN32)
    include(SymlinkHelpers)
    create_symlink_file_custom_command(${OLP_SDK_ANDROID_HTTP_CLIENT_JAR} POST_BUILD
        ${CMAKE_CURRENT_BINARY_DIR}/${OLP_SDK_ANDROID_HTTP_CLIENT_JAR}-${OLP_SDK_NETWORK_VERSION}.jar
        ${CMAKE_CURRENT_BINARY_DIR}/${OLP_SDK_ANDROID_HTTP_CLIENT_JAR}.jar)
endif()

if(FORCED_WIN32)
    unset(WIN32)
endif()
