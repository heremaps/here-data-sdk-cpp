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

project(dataservice-write-example VERSION 0.6.0)

add_executable(dataservice-write-example ${CMAKE_CURRENT_SOURCE_DIR}/example.h ${CMAKE_CURRENT_SOURCE_DIR}/example.cpp)

if (IOS)
    find_library(CORE_FOUNDATION Foundation)
    find_library(CF_NETWORK_FRAMEWORK CFNetwork)
    find_library(SECURITY_FRAMEWORK Security)
    find_library(UIKIT_FRAMEWORK UIKit)

    set(RESOURCE_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/ios/resources/Main.storyboard
                        ${CMAKE_CURRENT_SOURCE_DIR}/ios/resources/dummy.txt)
    target_sources(dataservice-write-example PUBLIC
                        ${CMAKE_CURRENT_SOURCE_DIR}/ios/AppDelegate.h
                        ${CMAKE_CURRENT_SOURCE_DIR}/ios/AppDelegate.mm
                        ${CMAKE_CURRENT_SOURCE_DIR}/ios/ViewController.h
                        ${CMAKE_CURRENT_SOURCE_DIR}/ios/ViewController.mm
                        ${CMAKE_CURRENT_SOURCE_DIR}/ios/main.mm
                        ${RESOURCE_FILES})
    set_property (TARGET dataservice-write-example APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")

    set(APP_BUNDLE_IDENTIFIER "com.here.olp.examples.dataservice-write")

    set(MACOSX_BUNDLE_INFO_STRING ${APP_BUNDLE_IDENTIFIER})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_BUNDLE_IDENTIFIER})
    set(MACOSX_BUNDLE_BUNDLE_NAME ${APP_BUNDLE_IDENTIFIER})
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "1.0")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "1.0")
    set_target_properties(dataservice-write-example PROPERTIES
        XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
        RESOURCE "${RESOURCE_FILES}"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    )

    target_link_libraries(dataservice-write-example
        ${CORE_FOUNDATION}
        ${SECURITY_FRAMEWORK}
        ${CF_NETWORK_FRAMEWORK}
        ${UIKIT_FRAMEWORK}
    )
else()
    target_sources(dataservice-write-example PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
endif()

target_link_libraries(dataservice-write-example
   olp-cpp-sdk-authentication
   olp-cpp-sdk-dataservice-write
)

