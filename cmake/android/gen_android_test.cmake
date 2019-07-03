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
function(gen_android_test_runner SERVICE_TARGET_NAME SERVICE_TEST_LIB_NAME)
    if(NOT ANDROID)
        return()
    endif()

    file(COPY "${CMAKE_SOURCE_DIR}/cmake/android/tester" DESTINATION
        "${CMAKE_CURRENT_BINARY_DIR}/android")
    file(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/android/${SERVICE_TEST_LIB_NAME}-tester")
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/android/tester/CMakeLists.txt.in")
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/android/tester/app/build.gradle.in")
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/android/tester/app/src/main/AndroidManifest.xml.in")
    file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/android/tester" "${CMAKE_CURRENT_BINARY_DIR}/android/${SERVICE_TEST_LIB_NAME}-tester")

    string(REGEX REPLACE "[\.-]" "" PACKAGE_NAME ${SERVICE_TEST_LIB_NAME})

    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/android/CMakeLists.txt.in
        ${CMAKE_CURRENT_BINARY_DIR}/android/CMakeLists.txt
        @ONLY
    )

    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/android/tester/CMakeLists.txt.in
        ${CMAKE_CURRENT_BINARY_DIR}/android/${SERVICE_TEST_LIB_NAME}-tester/CMakeLists.txt
        @ONLY
    )

    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/android/tester/app/build.gradle.in
        ${CMAKE_CURRENT_BINARY_DIR}/android/${SERVICE_TEST_LIB_NAME}-tester/app/build.gradle
        @ONLY
    )

    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/android/tester/app/src/main/AndroidManifest.xml.in
        ${CMAKE_CURRENT_BINARY_DIR}/android/${SERVICE_TEST_LIB_NAME}-tester/app/src/main/AndroidManifest.xml
        @ONLY
    )

    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/android/android_test.sh.in
        ${CMAKE_CURRENT_BINARY_DIR}/android/android_test.sh
        @ONLY
    )

endfunction()
