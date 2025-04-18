# Copyright (C) 2019-2023 HERE Europe B.V.
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

set(OLP_CPP_SDK_CORE_TESTS_SOURCES
    ./cache/DefaultCacheImplTest.cpp
    ./cache/DefaultCacheTest.cpp
    ./cache/Helpers.cpp
    ./cache/Helpers.h
    ./cache/InMemoryCacheTest.cpp
    ./cache/KeyGeneratorTest.cpp
    ./cache/ProtectedKeyListTest.cpp

    ./client/ApiLookupClientImplTest.cpp
    ./client/ApiResponseTest.cpp
    ./client/BackdownStrategyTest.cpp
    ./client/CancellationContextTest.cpp
    ./client/ConditionTest.cpp
    ./client/DefaultLookupEndpointProviderTest.cpp
    ./client/HRNTest.cpp
    ./client/OlpClientSettingsFactoryTest.cpp
    ./client/OlpClientTest.cpp
    ./client/PendingUrlRequestsTest.cpp
    ./client/TaskContextTest.cpp

    ./context/ContextTest.cpp

    ./geo/coordinates/GeoCoordinates3dTest.cpp
    ./geo/coordinates/GeoCoordinatesTest.cpp
    ./geo/coordinates/GeoPointTest.cpp
    ./geo/coordinates/GeoRectangleTest.cpp
    ./geo/projection/EquirectangularProjectionTest.cpp
    ./geo/projection/IdentityProjectionTest.cpp
    ./geo/projection/SphereProjectionTest.cpp
    ./geo/projection/WebMercatorProjectionTest.cpp
    ./geo/tiling/PathTilingTest.cpp
    ./geo/tiling/SubdivisionSchemeTest.cpp
    ./geo/tiling/SubTilesTest.cpp
    ./geo/tiling/TileKeyTest.cpp
    ./geo/tiling/TileKeyUtilsTest.cpp

    ./logging/ConfigurationTest.cpp
    ./logging/DisabledLoggingTest.cpp
    ./logging/FileAppenderTest.cpp
    ./logging/FilterGroupTest.cpp
    ./logging/FormatTest.cpp
    ./logging/LogTest.cpp
    ./logging/MessageFormatterTest.cpp
    ./logging/MockAppender.cpp

    ./thread/ContinuationTest.cpp
    ./thread/ExecutionContextTest.cpp
    ./thread/PriorityQueueExtendedTest.cpp
    ./thread/SyncQueueTest.cpp
    ./thread/TaskContinuationTest.cpp
    ./thread/ThreadPoolTaskSchedulerTest.cpp

    ./http/NetworkSettingsTest.cpp
    ./http/NetworkUtils.cpp

    ./utils/UtilsTest.cpp
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
            olp-cpp-sdk-tests-common
    )

    # Some tests include private headers of core. This should be fixed in tests,
    # only then here.
    target_include_directories(${OLP_SDK_CORE_TESTS_LIB}
        PRIVATE
            ../src/cache
            ../src/client
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
            olp-cpp-sdk-tests-common
    )
    # Some tests include private headers of core. This should be fixed in tests,
    # only then here.
    target_include_directories(olp-cpp-sdk-core-tests
    PRIVATE
        ../src/cache
    )

endif()
