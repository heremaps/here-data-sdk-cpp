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


project(olp-cpp-sdk-core VERSION 1.1.0)
set(DESCRIPTION "Core network and utility library for the HERE OLP SDK C++")

find_package(RapidJSON REQUIRED)
find_package(Boost REQUIRED)
find_package(leveldb REQUIRED)
find_package(Snappy REQUIRED)
find_package(Threads REQUIRED)

include(configs/ConfigNetwork.cmake)
include(cmake/CompileChecks.cmake)

porting_do_checks()

if(ANDROID OR IOS OR WIN32)
    set(NETWORK_NO_CURL ON)

    if(ANDROID)
        find_package(Java REQUIRED)
    endif()
else()
    set(NETWORK_NO_CURL OFF)

    find_package(CURL REQUIRED)
endif()


set(Network_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

include(cmake/curl.cmake)
include(cmake/winhttp.cmake)
include(cmake/android.cmake)
include(cmake/ios.cmake)

file(GLOB CACHE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/cache/*.h"
)

file(GLOB CLIENT_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/client/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/client/*.inl"
)

file(GLOB GENERATED_MODEL_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/generated/model/*.h"
    )

file(GLOB GENERATED_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/generated/model/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/generated/parser/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/generated/serializer/*.h"
)

set(OLP_SDK_HTTP_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/http/HttpStatusCode.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/http/Network.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/http/HttpStatusCode.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/http/NetworkConstants.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/http/NetworkProxySettings.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/http/NetworkRequest.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/http/NetworkResponse.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/http/NetworkSettings.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/http/NetworkTypes.h"
)
file(GLOB PLATFORM_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/context/*.h"
)
file(GLOB PORTING_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/porting/*.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/olp/core/porting/*.h"
)
file(GLOB UTILS_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/utils/*.h"
)
file(GLOB LOGGING_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/logging/*.h"
)
file(GLOB THREAD_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/thread/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/thread/*.inl"
)

file(GLOB GEOCOORDINATES_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/geo/coordinates/*.h"
)
file(GLOB GEOPROJECTION_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/geo/projection/*.h"
)
file(GLOB GEOTILING_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/geo/tiling/*.h"
)
file(GLOB GEOTYPE_HEADER
    "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/geo/*.h"
)

file(GLOB MATH_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/math/*.h")

file(GLOB COREAPI_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/olp/core/*.h")

file(GLOB_RECURSE GEO_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/geo/*.cpp"
)

file(GLOB CACHE_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cache/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cache/*.h"
)

file(GLOB CLIENT_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/client/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/client/*.h"
)

file(GLOB GENERATED_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/generated/parser/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/generated/serializer/*.cpp"
)

set(OLP_SDK_HTTP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/http/Network.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/http/NetworkProxySettings.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/http/NetworkRequest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/http/NetworkResponse.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/http/NetworkSettings.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/http/NetworkUtils.cpp"
)

if (ANDROID)

    # http network Android implementation:
    set(OLP_SDK_HTTP_ANDROID_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/android/NetworkAndroid.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/android/NetworkAndroid.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/android/utils/JNIThreadBinder.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/android/utils/JNIScopedLocalReference.h"
    )
    set(OLP_SDK_HTTP_SOURCES ${OLP_SDK_HTTP_SOURCES} ${OLP_SDK_HTTP_ANDROID_SOURCES})

    set(OLP_SDK_DEFAULT_NETWORK_DEFINITION NETWORK_HAS_ANDROID)
endif()
if (UNIX AND NOT ANDROID)
    set(OLP_SDK_HTTP_SOURCES ${OLP_SDK_HTTP_SOURCES} ${OLP_SDK_HTTP_CURL_SOURCES})

    set(OLP_SDK_DEFAULT_NETWORK_DEFINITION NETWORK_HAS_CURL)
endif()
if (IOS)
    set(OLP_SDK_HTTP_SOURCES ${OLP_SDK_HTTP_SOURCES} ${OLP_SDK_HTTP_IOS_SOURCES})

    set(OLP_SDK_DEFAULT_NETWORK_DEFINITION NETWORK_HAS_IOS)
endif()
if (WIN32)
    set(OLP_SDK_HTTP_SOURCES ${OLP_SDK_HTTP_SOURCES} ${OLP_SDK_HTTP_WIN_SOURCES})

    set(OLP_SDK_DEFAULT_NETWORK_DEFINITION NETWORK_HAS_WINHTTP)
endif()

file(GLOB PLATFORM_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/context/*.cpp"
)
file(GLOB UTILS_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.cpp"
)
file(GLOB LOGGING_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/logging/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/logging/*.h"
)
file(GLOB THREAD_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/thread/*.cpp"
)

file(GLOB CORE_HEADERS
    ${CACHE_HEADERS}
    ${CLIENT_HEADERS}
    ${GENERATED_HEADERS}
    ${OLP_SDK_HTTP_HEADERS}
    ${PLATFORM_HEADERS}
    ${PORTING_HEADERS}
    ${UTILS_HEADERS}
    ${LOGGING_HEADERS}
    ${THREAD_HEADERS}
    ${MATH_HEADERS}
    ${GEOCOORDINATES_HEADERS}
    ${GEOPROJECTION_HEADERS}
    ${GEOTILING_HEADERS}
    ${COREAPI_HEADER}
    ${GEOTYPE_HEADER}
)

file(GLOB CORE_SOURCE
    ${CACHE_SOURCE}
    ${CLIENT_SOURCE}
    ${GENERATED_SOURCE}
    ${OLP_SDK_HTTP_SOURCES}
    ${PLATFORM_SOURCE}
    ${UTILS_SOURCE}
    ${LOGGING_SOURCE}
    ${THREAD_SOURCE}
    ${GEO_SOURCE}
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

add_library(${PROJECT_NAME}
    ${CORE_SOURCE}
    ${CORE_HEADERS})

target_compile_definitions(${PROJECT_NAME}
    PRIVATE ${OLP_SDK_DEFAULT_NETWORK_DEFINITION})

if (OLP_SDK_NO_EXCEPTION AND NOT OLP_SDK_BOOST_THROW_EXCEPTION_EXTERNAL)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE OLP_SDK_BOOST_THROW_EXCEPTION=1)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${Boost_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${RapidJSON_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${leveldb_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>)

if (ANDROID)
    set(ADDITIONAL_LIBRARIES log)
else()
    set(ADDITIONAL_LIBRARIES)
endif()

if (HAVE_PTHREAD_SETNAME_NP)
    add_definitions(-DOLP_SDK_HAVE_PTHREAD_SETNAME_NP)
endif()

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${ADDITIONAL_LIBRARIES}
        ${NETWORK_CURL_LIBRARIES}
        ${NETWORK_WINHTTP_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
    PRIVATE
        leveldb::leveldb
)

if(IOS)
    if (CMAKE_GENERATOR MATCHES "Xcode")
        set_target_properties (
                ${PROJECT_NAME}
                PROPERTIES
                XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
        )
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE "-fobjc-arc")
    endif()

    find_library(OLP_SDK_CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(OLP_SDK_SECURITY_FRAMEWORK Security)
    find_library(OLP_SDK_CFNETWORK_FRAMEWORK CFNetwork)

    target_link_libraries(${PROJECT_NAME} PRIVATE ${OLP_SDK_CORE_FOUNDATION_FRAMEWORK}
                                                  ${OLP_SDK_SECURITY_FRAMEWORK}
                                                  ${OLP_SDK_CFNETWORK_FRAMEWORK})
elseif(ANDROID)
    # Make sure that OlpHttpClient.jar is built before olp-cpp-sdk-core
    add_dependencies(${PROJECT_NAME} ${OLP_SDK_ANDROID_HTTP_CLIENT_JAR})
endif()

if(CURL_FOUND)
    include(CheckIncludeFile)
    check_include_file(signal.h HAVE_SIGNAL_H)
    if(HAVE_SIGNAL_H)
        target_compile_definitions(${PROJECT_NAME}
            PRIVATE HAVE_SIGNAL_H IGNORE_SIGPIPE)
    endif()
endif()

option(NETWORK_SSL_VERIFY_OVERRIDE
    "Enable support for NETWORK_SSL_VERIFY environment variable to override SSL certificate verification" OFF)
if(NETWORK_SSL_VERIFY_OVERRIDE)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE NETWORK_SSL_VERIFY_OVERRIDE)
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE THREAD_LIBRARY)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE THREAD_LIBRARY_DYNAMIC)
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE CORE_LIBRARY)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC CORE_SHARED_LIBRARY)
endif()

# install component
install (FILES ${OLP_SDK_HTTP_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/http)
install (FILES ${PLATFORM_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/platform)
install (FILES ${PORTING_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/porting)
install (FILES ${UTILS_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/utils)
install (FILES ${THREAD_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/thread)
install (FILES ${CACHE_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/cache)
install (FILES ${CLIENT_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/client)
install (FILES ${GENERATED_MODEL_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/generated/model)
install (FILES ${MATH_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/math)
install (FILES ${GEOCOORDINATES_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/geo/coordinates)
install (FILES ${GEOPROJECTION_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/geo/projection)
install (FILES ${GEOTILING_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/geo/tiling)
install (FILES ${LOGGING_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/logging)
install (FILES ${COREAPI_HEADER} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/)
install (FILES ${GEOTYPE_HEADER} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/geo)

export_config()

if(OLP_SDK_ENABLE_TESTING)
    add_subdirectory(tests)
endif()
