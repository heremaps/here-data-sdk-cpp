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

if(CURL_FOUND AND NOT NETWORK_NO_CURL)
    set(OLP_SDK_HTTP_CURL_SOURCES ${OLP_SDK_HTTP_CURL_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/../src/http/curl/NetworkCurl.cpp)

    add_definitions(-DOLP_SDK_NETWORK_HAS_CURL)
    set(OLP_SDK_NETWORK_CURL_LIBRARIES ${CURL_LIBRARIES})
    if(WIN32 AND NOT MINGW)
        set(OLP_SDK_NETWORK_CURL_LIBRARIES ${OLP_SDK_NETWORK_CURL_LIBRARIES} ws2_32)
    endif()
    if(APPLE)
        set(OLP_SDK_NETWORK_CURL_LIBRARIES ${OLP_SDK_NETWORK_CURL_LIBRARIES} resolv)
    endif(APPLE)
    include_directories(${CURL_INCLUDE_DIRS})

    find_package(OpenSSL)
    if(OPENSSL_FOUND)
        add_definitions(-DOLP_SDK_NETWORK_HAS_OPENSSL)

        option(OLP_SDK_USE_LIBCRYPTO "Enables the libcrypto dependency" ON)

        if(OLP_SDK_USE_LIBCRYPTO)
            include_directories(${OPENSSL_INCLUDE_DIR})
            set(OLP_SDK_NETWORK_CURL_LIBRARIES ${OLP_SDK_NETWORK_CURL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARY})
        endif()
    endif()

    option(OLP_SDK_ENABLE_CURL_VERBOSE "Enable support for CURL_VERBOSE environment variable to set libcurl to verbose mode" OFF)
    if(OLP_SDK_ENABLE_CURL_VERBOSE)
        add_definitions(-DOLP_SDK_ENABLE_CURL_VERBOSE)
    endif()

    include(CheckSymbolExists)
    add_definitions(-DOLP_SDK_NETWORK_HAS_UNISTD_H=1)
    check_symbol_exists(pipe "unistd.h" OLP_SDK_HAS_PIPE)
    if(OLP_SDK_HAS_PIPE)
        add_definitions(-DOLP_SDK_NETWORK_HAS_PIPE=1)
    endif()
    check_symbol_exists(pipe2 "unistd.h" OLP_SDK_HAS_PIPE2)
    if(OLP_SDK_HAS_PIPE2)
        add_definitions(-DOLP_SDK_NETWORK_HAS_PIPE2=1)
    endif()

else()
    set(OLP_SDK_HTTP_CURL_SOURCES)
    set(OLP_SDK_NETWORK_CURL_LIBRARIES)
endif()
