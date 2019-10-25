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

set(OLP_SDK_FUNCTIONAL_TESTS_SOURCES
    ./olp-cpp-sdk-core/NetworkTest.cpp
    ./olp-cpp-sdk-core/OlpClientDefaultAsyncHttpTest.cpp
    ./olp-cpp-sdk-dataservice-read/ApiTest.cpp
    ./olp-cpp-sdk-dataservice-read/CatalogClientTest.cpp
    ./olp-cpp-sdk-dataservice-read/DataserviceReadVersionedLayerClientTest.cpp
    ./olp-cpp-sdk-dataservice-write/DataserviceWriteIndexLayerClientTest.cpp
    ./olp-cpp-sdk-dataservice-write/DataserviceWriteStreamLayerClientCacheTest.cpp
    ./olp-cpp-sdk-dataservice-write/DataserviceWriteStreamLayerClientTest.cpp
    ./olp-cpp-sdk-dataservice-write/DataserviceWriteVersionedLayerClientTest.cpp
    ./olp-cpp-sdk-dataservice-write/DataserviceWriteVolatileLayerClientTest.cpp
    ./olp-cpp-sdk-authentication/ArcGisAuthenticationTest.cpp
    ./olp-cpp-sdk-authentication/AuthenticationCommonTestFixture.cpp
    ./olp-cpp-sdk-authentication/AuthenticationClientTest.cpp
    ./olp-cpp-sdk-authentication/AuthenticationProductionTest.cpp
    ./olp-cpp-sdk-authentication/AuthenticationTestUtils.cpp
    ./olp-cpp-sdk-authentication/FacebookAuthenticationTest.cpp
    ./olp-cpp-sdk-authentication/GoogleAuthenticationTest.cpp
    ./olp-cpp-sdk-authentication/HereAccountOauth2ProductionTest.cpp
)

set(OLP_SDK_FUNCTIONAL_TESTS_HEADERS
    ./olp-cpp-sdk-authentication/AuthenticationCommonTestFixture.h
    ./olp-cpp-sdk-authentication/AuthenticationTestUtils.h
    ./olp-cpp-sdk-authentication/TestConstants.h
    ./utils/Utils.h
)

if (ANDROID OR IOS)
    add_library(olp-cpp-sdk-functional-tests-lib
        ${OLP_SDK_FUNCTIONAL_TESTS_SOURCES}
        ${OLP_SDK_FUNCTIONAL_TESTS_HEADERS})
    target_include_directories(olp-cpp-sdk-functional-lib
        PRIVATE
            ${PROJECT_SOURCE_DIR}/olp-cpp-sdk-dataservice-read/src
            utils
    )
    target_link_libraries(olp-cpp-sdk-functional-tests-lib
        PRIVATE
            custom-params
            gtest
            olp-cpp-sdk-tests-common
            olp-cpp-sdk-core
            olp-cpp-sdk-authentication
            olp-cpp-sdk-dataservice-read
            olp-cpp-sdk-dataservice-write
    )
    if (ANDROID)
        include(${CMAKE_SOURCE_DIR}/cmake/android/gen_android_test.cmake)
        gen_android_test_runner(olp-cpp-sdk-functional-tests
            olp-cpp-sdk-functional-tests-lib)
        add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/android
            ${CMAKE_CURRENT_BINARY_DIR}/android)

    else()
        include(${CMAKE_SOURCE_DIR}/cmake/ios/gen_ios_test.cmake)
        gen_ios_test_runner(olp-cpp-sdk-functional-tests olp-cpp-sdk-functional-tests-lib)
        add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/ios
            ${CMAKE_CURRENT_BINARY_DIR}/ios)

    endif()

else()
    add_executable(olp-cpp-sdk-functional-tests
        ${OLP_SDK_FUNCTIONAL_TESTS_SOURCES}
        ${OLP_SDK_FUNCTIONAL_TESTS_HEADERS})
    target_include_directories(olp-cpp-sdk-functional-tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/olp-cpp-sdk-dataservice-read/src
        utils
    )
    target_link_libraries(olp-cpp-sdk-functional-tests
        PRIVATE
            custom-params
            gtest_main
            olp-cpp-sdk-tests-common
            olp-cpp-sdk-core
            olp-cpp-sdk-authentication
            olp-cpp-sdk-dataservice-read
            olp-cpp-sdk-dataservice-write
    )
endif()
