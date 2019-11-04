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

set(OLP_CPP_SDK_CORE_TESTS_SOURCES
    ./cache/DefaultCacheTest.cpp
    ./cache/InMemoryCacheTest.cpp

    ./client/CancellationContextTest.cpp
    ./client/ConditionTest.cpp

    ./geo/coordinates/GeoCoordinates3dTest.cpp
    ./geo/coordinates/GeoCoordinatesTest.cpp
    ./geo/coordinates/GeoPointTest.cpp
    ./geo/coordinates/GeoRectangleTest.cpp
    ./geo/projection/EquirectangularProjectionTest.cpp
    ./geo/projection/IdentityProjectionTest.cpp
    ./geo/projection/SphereProjectionTest.cpp
    ./geo/projection/WebMercatorProjectionTest.cpp
    ./geo/tiling/SubdivisionSchemeTest.cpp
    ./geo/tiling/SubTilesTest.cpp
    ./geo/tiling/TileKeyTest.cpp
    ./geo/tiling/TileKeyUtilsTest.cpp

    ./olpclient/OlpClientTest.cpp

    ./thread/SyncQueueTest.cpp
    ./thread/ThreadPoolTaskSchedulerTest.cpp
)

if (ANDROID OR IOS)
    set(OLP_SDK_CORE_TESTS_LIB olp-cpp-sdk-core-tests-lib)

    add_library(${OLP_SDK_CORE_TESTS_LIB} ${OLP_CPP_SDK_CORE_TESTS_SOURCES})
    target_link_libraries(${OLP_SDK_CORE_TESTS_LIB}
        PRIVATE
            custom-params
            gmock
            gtest
            olp-cpp-sdk-core
    )

    # Some tests include private headers of core. This should be fixed in tests,
    # only then here.
    target_include_directories(${OLP_SDK_CORE_TESTS_LIB}
        PRIVATE
            ../src/cache
    )
    if (ANDROID)
        include(${CMAKE_SOURCE_DIR}/cmake/android/gen_android_test.cmake)
        gen_android_test_runner(olp-cpp-sdk-core-tests
            ${OLP_SDK_CORE_TESTS_LIB})
        add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/android
            ${CMAKE_CURRENT_BINARY_DIR}/android)

    else()
        include(${CMAKE_SOURCE_DIR}/cmake/ios/gen_ios_test.cmake)
        gen_ios_test_runner(olp-cpp-sdk-core-tests ${OLP_SDK_CORE_TESTS_LIB})
        add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/ios
            ${CMAKE_CURRENT_BINARY_DIR}/ios)

    endif()

else()
    add_executable(olp-cpp-sdk-core-tests ${OLP_CPP_SDK_CORE_TESTS_SOURCES})
    target_link_libraries(olp-cpp-sdk-core-tests
        PRIVATE
            custom-params
            gmock
            gtest
            gtest_main
            olp-cpp-sdk-core
    )
    # Some tests include private headers of core. This should be fixed in tests,
    # only then here.
    target_include_directories(olp-cpp-sdk-core-tests
    PRIVATE
        ../src/cache
    )

endif()
