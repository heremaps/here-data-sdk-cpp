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
find_package(Boost REQUIRED COMPONENTS program_options)

set(OLP_DATASERVICE_WRITE_LIB olp-dataservice-write)
set(OLP_DATASERVICE_WRITE_TEST ${OLP_DATASERVICE_WRITE_LIB}-test)
set(OLP_DATASERVICE_WRITE_TEST_SRC
    unit/HttpResponses.h
    unit/TestParser.cpp
    unit/TestSerializer.cpp
    unit/TestStreamLayerClient.cpp
    unit/TestThreadSafeQueue.cpp
    unit/TestTimeUtils.cpp
    unit/TestVersionedLayerClient.cpp
    unit/TestVolatileLayerClient.cpp
    unit/TestCancellationTokenList.cpp
    unit/TestIndexLayerClient.cpp)

if (ANDROID OR IOS)
    add_library(${OLP_DATASERVICE_WRITE_TEST}
        ${OLP_DATASERVICE_WRITE_TEST_SRC}
    )
else()
    add_executable(${OLP_DATASERVICE_WRITE_TEST}
        ${OLP_DATASERVICE_WRITE_TEST_SRC}
        DataserviceWriteTestMain.cpp)
endif()

target_include_directories(${OLP_DATASERVICE_WRITE_TEST}
   PRIVATE ${Boost_INCLUDE_DIRS}
   # For internal testing
   PRIVATE ${${PROJECT_NAME}_SOURCE_DIR}/src)

# For checksum testing
find_package(OpenSSL)
if(OPENSSL_FOUND)
    target_include_directories(olp-dataservice-write-test
        PRIVATE ${OPENSSL_INCLUDE_DIR})

    target_link_libraries(olp-dataservice-write-test
        PRIVATE ${OPENSSL_CRYPTO_LIBRARY})

    target_compile_definitions(olp-dataservice-write-test
        PRIVATE -DDATASERVICE_WRITE_HAS_OPENSSL)
endif()

if (ANDROID OR IOS)
    # Uses custom test runner for Android.
    target_link_libraries(${OLP_DATASERVICE_WRITE_TEST}
        PRIVATE olp-cpp-sdk-authentication
        PRIVATE olp-cpp-sdk-dataservice-write
        PRIVATE ${Boost_LIBRARIES}
        PUBLIC gmock
        PUBLIC custom-params)

    if (ANDROID)
        # Create Android test runner app
        include(${CMAKE_SOURCE_DIR}/cmake/android/gen_android_test.cmake)
        gen_android_test_runner(${OLP_DATASERVICE_WRITE_LIB} ${OLP_DATASERVICE_WRITE_TEST})

        add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/android ${CMAKE_CURRENT_BINARY_DIR}/android)
    else()
        # Create iOS test runner app
        include(${CMAKE_SOURCE_DIR}/cmake/ios/gen_ios_test.cmake)
        gen_ios_test_runner(${OLP_DATASERVICE_WRITE_LIB} ${OLP_DATASERVICE_WRITE_TEST})

        add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/ios ${CMAKE_CURRENT_BINARY_DIR}/ios)
    endif()
else()
    target_link_libraries(${OLP_DATASERVICE_WRITE_TEST}
        PRIVATE olp-cpp-sdk-authentication
        PRIVATE olp-cpp-sdk-dataservice-write
        PRIVATE gmock_main
        PRIVATE ${Boost_LIBRARIES}
        PRIVATE custom-params)
endif()
