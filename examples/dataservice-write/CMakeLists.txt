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


set(EDGE_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET dataservice-write-example)
project(${EDGE_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET} VERSION 0.6.0)

if (IOS)
    add_executable(${EDGE_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/example.h ${CMAKE_CURRENT_SOURCE_DIR}/example.cpp)

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
    set_property (TARGET ${EDGE_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET} APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")

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

    target_link_libraries(${EDGE_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET}
        ${CORE_FOUNDATION}
        ${SECURITY_FRAMEWORK}
        ${CF_NETWORK_FRAMEWORK}
        ${UIKIT_FRAMEWORK}
    )
elseif(ANDROID)
    # Copy all the files from helpers directory except the *.in files
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../helpers/android/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/android)
    file(GLOB_RECURSE EXCLUDED_IN_FILES ${CMAKE_CURRENT_BINARY_DIR}/android/*.in)
    file(REMOVE ${EXCLUDED_IN_FILES})

    # Configure all the variables required to generated gralde project
    set(EDGE_SDK_EXAMPLE_PACKAGE_NAME "dataservice_write_example")
    set(EDGE_SDK_EXAMPLE_PACKAGE_JNI_NAME "dataservice_1write_1example")
    set(EDGE_SDK_EXAMPLE_TARGET_NAME "dataservice-write-example-lib")

    set(EDGE_SDK_EXAMPLE_ANDROID_ABI "// abiFilters 'arm64-v8a'")
    if (DEFINED ANDROID_ABI)
        set(EDGE_SDK_EXAMPLE_ANDROID_ABI "abiFilters '${ANDROID_ABI}'")
    endif()

    set(EDGE_SDK_EXAMPLE_ANDROID_TOOLCHAIN_FILE "-DCMAKE_TOOLCHAIN_FILE='path to your toolchain file'")
    if (DEFINED CMAKE_TOOLCHAIN_FILE)
        set(EDGE_SDK_EXAMPLE_ANDROID_TOOLCHAIN_FILE "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
    endif()

    set(EDGE_SDK_EXAMPLE_NETWORK_PROTOCOL_JAR "-DEDGE_SDK_NETWORK_PROTOCOL_JAR='full path to the EdgeNetworkProtocol.jar in the <olp-edge-sdk-build-folder>/olp-cpp-sdk-core/ folder'")
    if (DEFINED olp-cpp-sdk-core_BINARY_DIR)
        set(EDGE_SDK_EXAMPLE_NETWORK_PROTOCOL_JAR "-DEDGE_SDK_NETWORK_PROTOCOL_JAR=${olp-cpp-sdk-core_BINARY_DIR}/EdgeNetworkProtocol.jar")
    endif()

    set(EDGE_SDK_EXAMPLE_LEVELDB_DIR "-Dleveldb_DIR='path to the directory which contains LevelDB cmake config files'")
    if (DEFINED leveldb_DIR)
        set(EDGE_SDK_EXAMPLE_LEVELDB_DIR "-Dleveldb_DIR=${leveldb_DIR}")
    endif()

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../helpers/android/app/build.gradle.in
        ${CMAKE_CURRENT_BINARY_DIR}/android/app/build.gradle
        @ONLY
    )

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../helpers/android/app/CMakeLists.txt.in
        ${CMAKE_CURRENT_BINARY_DIR}/android/app/CMakeLists.txt
        @ONLY
    )

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../helpers/android/app/src/main/AndroidManifest.xml.in
        ${CMAKE_CURRENT_BINARY_DIR}/android/app/src/main/AndroidManifest.xml
        @ONLY
    )

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../helpers/android/app/src/main/cpp/MainActivityNative.cpp.in
        ${CMAKE_CURRENT_BINARY_DIR}/android/app/src/main/cpp/MainActivityNative.cpp
        @ONLY
    )

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../helpers/android/app/src/main/java/MainActivity.java.in
        ${CMAKE_CURRENT_BINARY_DIR}/android/app/src/main/java/com/here/android/olp/${EDGE_SDK_EXAMPLE_PACKAGE_NAME}/MainActivity.java
        @ONLY
    )

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../helpers/android/app/src/main/res/values/strings.xml.in
        ${CMAKE_CURRENT_BINARY_DIR}/android/app/src/main/res/values/strings.xml
        @ONLY
    )

    # Symlink (or copy - depending on the platform) the example.h / example.cpp files to the gradle fodler
    set(EDGE_SDK_EXAMPLE_H_FILE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/example.h)
    set(EDGE_SDK_EXAMPLE_H_FILE_DST ${CMAKE_CURRENT_BINARY_DIR}/android/app/src/main/cpp/example.h)
    set(EDGE_SDK_EXAMPLE_CPP_FILE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/example.cpp)
    set(EDGE_SDK_EXAMPLE_CPP_FILE_DST ${CMAKE_CURRENT_BINARY_DIR}/android/app/src/main/cpp/example.cpp)

    if(CMAKE_HOST_WIN32 AND NOT CYGWIN AND NOT MSYS)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${EDGE_SDK_EXAMPLE_H_FILE_SRC} ${EDGE_SDK_EXAMPLE_H_FILE_DST})
        execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${EDGE_SDK_EXAMPLE_CPP_FILE_SRC} ${EDGE_SDK_EXAMPLE_CPP_FILE_DST})
    else(CMAKE_HOST_WIN32 AND NOT CYGWIN AND NOT MSYS)
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${EDGE_SDK_EXAMPLE_H_FILE_SRC} ${EDGE_SDK_EXAMPLE_H_FILE_DST})
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${EDGE_SDK_EXAMPLE_CPP_FILE_SRC} ${EDGE_SDK_EXAMPLE_CPP_FILE_DST})
    endif(CMAKE_HOST_WIN32 AND NOT CYGWIN AND NOT MSYS)

else()
    add_executable(${EDGE_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/example.h ${CMAKE_CURRENT_SOURCE_DIR}/example.cpp)

    target_sources(${EDGE_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

    target_link_libraries(${EDGE_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET}
        olp-cpp-sdk-authentication
        olp-cpp-sdk-dataservice-write
    )
endif()