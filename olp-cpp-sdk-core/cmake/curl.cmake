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

if(CURL_FOUND AND NOT NETWORK_NO_CURL)
    aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../src/network/curl NETWORK_CURL_SOURCES)
    set(NETWORK_CURL_SOURCES ${NETWORK_CURL_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/../src/network/socket/NetworkConnectivitySocketImpl.cpp)
    set(EDGE_SDK_HTTP_CURL_SOURCES ${EDGE_SDK_HTTP_CURL_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/../src/http/curl/NetworkCurl.cpp)

    add_definitions(-DNETWORK_HAS_CURL)
    set(NETWORK_CURL_LIBRARIES ${CURL_LIBRARIES})
    if(WIN32)
        set(NETWORK_CURL_LIBRARIES ${NETWORK_CURL_LIBRARIES} ws2_32)
    endif()
    if(APPLE)
        set(NETWORK_CURL_LIBRARIES ${NETWORK_CURL_LIBRARIES} resolv)
    endif(APPLE)
    include_directories(${CURL_INCLUDE_DIRS})

    find_package(OpenSSL)
    if(OPENSSL_FOUND)
        add_definitions(-DNETWORK_HAS_OPENSSL)
        include_directories(${OPENSSL_INCLUDE_DIR})
        set(NETWORK_CURL_LIBRARIES ${NETWORK_CURL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARY})
    endif()

    option(ENABLE_CURL_VERBOSE "Enable support for CURL_VERBOSE environment variable to set libcurl to verbose mode" OFF)
    if(ENABLE_CURL_VERBOSE)
        add_definitions(-DENABLE_CURL_VERBOSE)
    endif()

    include(CheckFunctionExists)
    add_definitions(-DNETWORK_HAS_UNISTD_H=1)
    check_function_exists("pipe" HAS_PIPE)
    if(HAS_PIPE)
        add_definitions(-DNETWORK_HAS_PIPE=1)
    endif()
    check_function_exists("pipe2" HAS_PIPE2)
    if(HAS_PIPE2)
        add_definitions(-DNETWORK_HAS_PIPE2=1)
    endif()

else()
    set(NETWORK_CURL_SOURCES)
    set(NETWORK_CURL_LIBRARIES)
endif()
