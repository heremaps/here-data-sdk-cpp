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
if(NOT ANDROID)
    set(NETWORK_ANDROID_SOURCES)
    set(NETWORK_ANDROID_LIBRARIES)
    return()
endif()

aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../src/network/android NETWORK_ANDROID_SOURCES)
set(NETWORK_ANDROID_SOURCES ${NETWORK_ANDROID_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/../src/network/socket/NetworkConnectivitySocketImpl.cpp)
add_definitions(-DNETWORK_HAS_ANDROID)
set(NETWORK_ANDROID_LIBRARIES ${ANDROID_LIBRARY})
include_directories(${ANDROID_INCLUDE_DIR})

include(UseJava)

# cmake wants to always use posix path separators due to the android ndk toolchain
# This does not work with nmake on Windows. Force WIN32
if(CMAKE_HOST_WIN32 AND NOT WIN32)
    set(FORCED_WIN32 TRUE)
    set(WIN32 TRUE)
endif()

# Generation of both version of EdgeNetworkProtocol.jar:
#   - EdgeNetworkProtocol.jar, which uses old network implementation
#   - EdgeHttpNetworkProtocol.jar, which uses new http network implementation
# Currently across the SDK will be used the old version od EdgeNetworkProtocol.jar
# In order to use the new http version of EdgeNetworkProtocol.jar, please pass
# the EDGE_SDK_USE_HTTP_NETWORK_PROTOCOL=YES to your cmake configure command.
set(CMAKE_JAVA_COMPILE_FLAGS -source 1.7 -target 1.7)
set(CMAKE_JAR_CLASSES_PREFIX com/here/olp/network)

include(${CMAKE_CURRENT_LIST_DIR}/GetAndroidVariables.cmake)
get_android_jar_path(CMAKE_JAVA_INCLUDE_PATH SDK_ROOT ANDROID_PLATFORM)

# Generating of both old network (EdgeNetworkProtocol.jar, based on the previous version of network)
# and the new network (EdgeHttpClient, based on the http network implementation) jar archives.
# By defaut the old implementation will be used across the SDK.
set(EDGE_SDK_NETWORK_VERSION 0.0.1)

set(EDGE_NETWORK_PROTOCOL_JAR EdgeNetworkProtocol)
set(EDGE_SDK_ANDROID_HTTP_CLIENT_JAR EdgeHttpClient)

add_jar(${EDGE_NETWORK_PROTOCOL_JAR}
    SOURCES ${CMAKE_CURRENT_LIST_DIR}/../src/network/android/NetworkProtocol.java ${CMAKE_CURRENT_LIST_DIR}/../src/network/android/NetworkSSLContextFactory.java
    VERSION ${EDGE_SDK_NETWORK_VERSION}
)
add_jar(${EDGE_SDK_ANDROID_HTTP_CLIENT_JAR}
    SOURCES ${CMAKE_CURRENT_LIST_DIR}/../src/http/android/HttpClient.java
    VERSION ${EDGE_SDK_NETWORK_VERSION}
)

# add_jar() doesn't add the symlink to the version automatically under Windows
if (WIN32)
    include(SymlinkHelpers)
    create_symlink_file_custom_command(${EDGE_NETWORK_PROTOCOL_JAR} POST_BUILD
        ${CMAKE_CURRENT_BINARY_DIR}/${EDGE_NETWORK_PROTOCOL_JAR}-${NETWORK_VERSION}.jar
        ${CMAKE_CURRENT_BINARY_DIR}/${EDGE_NETWORK_PROTOCOL_JAR}.jar)
endif()

if(FORCED_WIN32)
    unset(WIN32)
endif()
