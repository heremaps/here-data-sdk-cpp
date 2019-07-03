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
set(OLP-AUTHENTICATION-LIB olp-authentication)
set(OLP-AUTHENTICATION-TEST ${OLP-AUTHENTICATION-LIB}-test)

set(OLP_AUTHENTICATION_TEST_SRC
    ArcGisTestUtils.cpp
    ArcGisTestUtils.h
    AuthenticationProductionTests.cpp
    AuthenticationTests.h
    AuthenticationTests.cpp
    AuthenticationUtils.cpp
    AuthenticationUtils.h
    CommonTestUtils.h
    FacebookTestUtils.cpp
    FacebookTestUtils.h
    GoogleTestUtils.cpp
    GoogleTestUtils.h
    OfflineResponses.h
    TestHereAccountOauth2IntegrationOnline.cpp
)

if(ANDROID OR IOS)
    add_library(${OLP-AUTHENTICATION-TEST}
        ${OLP_AUTHENTICATION_TEST_SRC}
    )
else()
    add_executable(${OLP-AUTHENTICATION-TEST}
        ${OLP_AUTHENTICATION_TEST_SRC}
        main.cpp
    )
endif()

get_property(NETWORK_TEST_DIR GLOBAL PROPERTY olp.network_tests_dir)

target_include_directories(${OLP-AUTHENTICATION-TEST}
    # need the src dir for internal testing
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../src"
    PRIVATE ${NETWORK_TEST_DIR}/unittest # Need to use the MockTestConfig.h
    PRIVATE ${OLP_COMMON_TEST_HEADERS_DIR}
)

target_link_libraries(${OLP-AUTHENTICATION-TEST}
    olp-cpp-sdk-authentication
    olp-cpp-sdk-core
    custom-params
)

if(ANDROID)
    # Uses custom test runner for Android.
    target_link_libraries(${OLP-AUTHENTICATION-TEST} gmock)

    # Create Android test runner app
    include(${CMAKE_SOURCE_DIR}/cmake/android/gen_android_test.cmake)
    gen_android_test_runner(${OLP-AUTHENTICATION-LIB} ${OLP-AUTHENTICATION-TEST})

    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/android ${CMAKE_CURRENT_BINARY_DIR}/android)
elseif(IOS)
    # Create iOS test runner app
    include(${CMAKE_SOURCE_DIR}/cmake/ios/gen_ios_test.cmake)
    gen_ios_test_runner(${OLP-AUTHENTICATION-LIB} ${OLP-AUTHENTICATION-TEST})

    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/ios ${CMAKE_CURRENT_BINARY_DIR}/ios)
else()
    target_link_libraries(${OLP-AUTHENTICATION-TEST} gmock_main)
endif()

