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
function(gen_ios_test)
    if(NOT IOS)
        return()
    endif()

    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/ios/build_testapp.sh.in
        ./build_testapp.sh
        @ONLY
    )

    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/ios/ios_test.sh.in
        ./ios_test.sh
        @ONLY
    )
endfunction()


function(gen_ios_test_runner SERVICE_NAME SERVICE_TEST_LIB)
    if(NOT IOS)
        return()
    endif()

    if (NOT IOS_TEST_DIR)
        set (IOS_TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/ios")
    endif()
    message("Copying ios folder")
    file(COPY "${CMAKE_SOURCE_DIR}/cmake/ios/tester" DESTINATION
        "${IOS_TEST_DIR}")
    file(REMOVE_RECURSE "${IOS_TEST_DIR}/${SERVICE_NAME}-tester")
    file(REMOVE "${IOS_TEST_DIR}/tester/CMakeLists.txt.in")
    file(RENAME "${IOS_TEST_DIR}/tester" "${IOS_TEST_DIR}/${SERVICE_NAME}-tester")

    message("Copying CMakeLists.txt")
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/ios/CMakeLists.txt.in
        ${IOS_TEST_DIR}/CMakeLists.txt
        @ONLY
    )

    string(TOUPPER ${SERVICE_NAME} SERVICE_NAME_UPPER)
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/ios/tester/CMakeLists.txt.in
        ${IOS_TEST_DIR}/${SERVICE_NAME}-tester/CMakeLists.txt
        @ONLY
    )
endfunction()
