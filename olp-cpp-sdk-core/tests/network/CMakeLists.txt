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

set_property(GLOBAL PROPERTY olp.network_tests_dir ${CMAKE_CURRENT_SOURCE_DIR})

find_package(valgrind QUIET)

set(NETWORK_TEST_TARGETS)

function(add_unit_test_net LIBNAME FILES)
    _add_unit_test_net(${LIBNAME} "${FILES}")

    target_compile_definitions(${LIBNAME}-test PRIVATE
        NETWORK_USE_MEMORY_LIB=${NETWORK_USE_MEMORY_LIB})

    # propagate NETWORK_TEST_TARGETS to the global scope
    set(NETWORK_TEST_TARGETS ${NETWORK_TEST_TARGETS} PARENT_SCOPE)
endfunction(add_unit_test_net)

function(_add_unit_test_net TARGET FILES)
    if (ANDROID OR IOS)
        add_library(${TARGET}-test ${FILES})
    else()
        add_executable(${TARGET}-test ${FILES})
    endif()

    target_include_directories(${TARGET}-test
        PRIVATE ${PROJECT_SOURCE_DIR}/src/network
        PRIVATE ${PROJECT_SOURCE_DIR}/tests)

    target_link_libraries(${TARGET}-test
        olp-cpp-sdk-core
        custom-params
    )

    if (ANDROID OR IOS)
        # Uses custom test runner for Android.
        target_link_libraries(${TARGET}-test gtest gmock)
    else()
        target_link_libraries(${TARGET}-test gtest_main gmock_main)
    endif()

    add_test(NAME ${TARGET}-test COMMAND ${TARGET}-test)

    set(NETWORK_TEST_TARGETS ${NETWORK_TEST_TARGETS} ${TARGET}-test PARENT_SCOPE)

    if (ANDROID)
        # Create Android test runner app
        include(${CMAKE_SOURCE_DIR}/cmake/android/gen_android_test.cmake)
        gen_android_test_runner(${TARGET} ${TARGET}-test)

        add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/android ${CMAKE_CURRENT_BINARY_DIR}/android)
    elseif (IOS)
        # Create iOS test runner app
        include(${CMAKE_SOURCE_DIR}/cmake/ios/gen_ios_test.cmake)
        gen_ios_test_runner(${TARGET} ${TARGET}-test)

        add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/ios ${CMAKE_CURRENT_BINARY_DIR}/ios)
    endif()
endfunction(_add_unit_test_net)

## unit tests (Name Source CollectMetrics)
set(UNITTEST_FILES)
set(UNITTEST_FILES ${UNITTEST_FILES}
                    unittest/TestNetworkRequest.cpp
                    unittest/TestNetworkResponse.cpp
                    unittest/TestNetworkRequestPriorityQueueMultithreading.cpp
                    unittest/TestNetworkStream.cpp
                    unittest/TestNetworkStreamBuf.cpp)
if(NOT EMSCRIPTEN)
    set(UNITTEST_FILES ${UNITTEST_FILES}
                    unittest/TestNetworkRequestPriorityQueue.cpp
                    unittest/TestNetworkPriorityQueueDecorator.cpp
                    unittest/TestNetworkPriorityQueueDecoratorMultithreading.cpp)
endif()

add_unit_test_net(olp-core-network "${UNITTEST_FILES}")

