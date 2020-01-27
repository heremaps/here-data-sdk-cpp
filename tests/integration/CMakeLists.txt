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

set(OLP_SDK_INTEGRATIONAL_TESTS_SOURCES
    ./olp-cpp-sdk-authentication/AuthenticationClientTest.cpp
    ./olp-cpp-sdk-authentication/HereAccountOauth2Test.cpp
    ./olp-cpp-sdk-dataservice-read/CatalogClientCacheTest.cpp
    ./olp-cpp-sdk-dataservice-read/VolatileLayerClientCacheTest.cpp
    ./olp-cpp-sdk-dataservice-read/VersionedLayerClientCacheTest.cpp
    ./olp-cpp-sdk-dataservice-read/CatalogClientTestBase.cpp
    ./olp-cpp-sdk-dataservice-read/CatalogClientTest.cpp
    ./olp-cpp-sdk-dataservice-read/HttpResponses.h
    ./olp-cpp-sdk-dataservice-read/StreamLayerClientTest.cpp
    ./olp-cpp-sdk-dataservice-read/VersionedLayerClientTest.cpp
    ./olp-cpp-sdk-dataservice-read/VolatileLayerClientTest.cpp
    ./olp-cpp-sdk-dataservice-write/HttpResponses.h
    ./olp-cpp-sdk-dataservice-write/IndexLayerClientTest.cpp
    ./olp-cpp-sdk-dataservice-write/StreamLayerClientCacheTest.cpp
    ./olp-cpp-sdk-dataservice-write/StreamLayerClientTest.cpp
    ./olp-cpp-sdk-dataservice-write/VolatileLayerClientTest.cpp
)

if (ANDROID OR IOS)
    set(OLP_SDK_INTEGRATION_TESTS_LIB olp-cpp-sdk-integration-tests-lib)

    add_library(${OLP_SDK_INTEGRATION_TESTS_LIB} ${OLP_SDK_INTEGRATIONAL_TESTS_SOURCES})
    target_link_libraries(${OLP_SDK_INTEGRATION_TESTS_LIB}
        PRIVATE
            gmock
            olp-cpp-sdk-tests-common
            olp-cpp-sdk-core
            olp-cpp-sdk-authentication
            olp-cpp-sdk-dataservice-read
            olp-cpp-sdk-dataservice-write
            olp-cpp-sdk-tests-common
    )
    if (ANDROID)
        include(${CMAKE_SOURCE_DIR}/cmake/android/gen_android_test.cmake)
        gen_android_test_runner(olp-cpp-sdk-integration-tests
            ${OLP_SDK_INTEGRATION_TESTS_LIB})
        add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/android
            ${CMAKE_CURRENT_BINARY_DIR}/android)

    else()
        include(${CMAKE_SOURCE_DIR}/cmake/ios/gen_ios_test.cmake)
        gen_ios_test_runner(olp-cpp-sdk-integration-tests ${OLP_SDK_INTEGRATION_TESTS_LIB})
        add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/ios
            ${CMAKE_CURRENT_BINARY_DIR}/ios)

    endif()

else()
    add_executable(olp-cpp-sdk-integration-tests ${OLP_SDK_INTEGRATIONAL_TESTS_SOURCES})
    target_link_libraries(olp-cpp-sdk-integration-tests
        PRIVATE
            gmock_main
            olp-cpp-sdk-tests-common
            olp-cpp-sdk-core
            olp-cpp-sdk-authentication
            olp-cpp-sdk-dataservice-read
            olp-cpp-sdk-dataservice-write
            olp-cpp-sdk-tests-common
    )
endif()
