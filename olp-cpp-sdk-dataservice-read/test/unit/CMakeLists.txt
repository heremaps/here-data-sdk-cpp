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

set(OLP-DS-READ-LIB olp-dataservice-read)
set(OLP-DS-READ-TEST ${OLP-DS-READ-LIB}-test)

set(TEST_SRC
    src/ApiTest.cpp
    src/CatalogClientTest.cpp
    src/HttpResponses.h
    src/MultiRequestContextTest.cpp
    src/ParserTest.cpp
    src/PendingRequestsTest.cpp
    src/SerializerTest.cpp
)

if(ANDROID OR IOS)
    add_library(${OLP-DS-READ-TEST}
        ${TEST_SRC}
    )
else()
    add_executable(${OLP-DS-READ-TEST}
        ${TEST_SRC}
        src/main.cpp
)
endif()

target_include_directories(${OLP-DS-READ-TEST}
   # For internal testing
   PRIVATE ${${PROJECT_NAME}_SOURCE_DIR}/src
)

target_link_libraries(${OLP-DS-READ-TEST}
   olp-cpp-sdk-dataservice-read
   olp-cpp-sdk-authentication
   custom-params
)

if(ANDROID)
    # Uses custom test runner for Android.
    target_link_libraries(${OLP-DS-READ-TEST} gmock)

    # Create Android test runner app
    include(${CMAKE_SOURCE_DIR}/cmake/android/gen_android_test.cmake)
    gen_android_test_runner(${OLP-DS-READ-LIB} ${OLP-DS-READ-TEST})

    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/android ${CMAKE_CURRENT_BINARY_DIR}/android)
elseif(IOS)
    # Create iOS test runner app
    include(${CMAKE_SOURCE_DIR}/cmake/ios/gen_ios_test.cmake)
    gen_ios_test_runner(${OLP-DS-READ-LIB} ${OLP-DS-READ-TEST})

    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/ios ${CMAKE_CURRENT_BINARY_DIR}/ios)
else()
    target_link_libraries(${OLP-DS-READ-TEST} gmock_main)
endif()
