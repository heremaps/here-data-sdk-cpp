# Copyright (C) 2019-2021 HERE Europe B.V.
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


project(olp-cpp-sdk-core VERSION 1.14.0)
set(DESCRIPTION "Core network and utility library for the HERE OLP SDK C++")

find_package(RapidJSON 1.1.0 REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

if(OLP_SDK_ENABLE_DEFAULT_CACHE)
    find_package(Snappy REQUIRED)
    find_package(leveldb REQUIRED)
endif()

if(OLP_SDK_ENABLE_DEFAULT_CACHE_LMDB)
    find_package(lmdb REQUIRED)
endif()

configure_file(./include/olp/core/Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/olp/core/Config.h)

include(configs/ConfigNetwork.cmake)
include(cmake/CompileChecks.cmake)

porting_do_checks()

if(IOS OR (WIN32 AND NOT MINGW))
    set(NETWORK_NO_CURL ON)
elseif(ANDROID)
    if(OLP_SDK_ENABLE_ANDROID_CURL)
        set(NETWORK_NO_CURL OFF)
        find_package(CURL 7.52.0 REQUIRED)
    else()
        find_package(Java REQUIRED)
        set(NETWORK_NO_CURL ON)
    endif()
else()
    set(NETWORK_NO_CURL OFF)
    find_package(CURL 7.52.0 REQUIRED)
endif()

set(Network_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

include(cmake/curl.cmake)
include(cmake/winhttp.cmake)
include(cmake/android.cmake)
include(cmake/ios.cmake)

set(OLP_SDK_CACHE_HEADERS
    ./include/olp/core/cache/CacheSettings.h
    ./include/olp/core/cache/DefaultCache.h
    ./include/olp/core/cache/KeyGenerator.h
    ./include/olp/core/cache/KeyValueCache.h
)

set(OLP_SDK_CLIENT_HEADERS
    ./include/olp/core/client/ApiError.h
    ./include/olp/core/client/ApiLookupClient.h
    ./include/olp/core/client/ApiNoResult.h
    ./include/olp/core/client/ApiResponse.h
    ./include/olp/core/client/BackdownStrategy.h
    ./include/olp/core/client/CancellationContext.h
    ./include/olp/core/client/CancellationContext.inl
    ./include/olp/core/client/CancellationToken.h
    ./include/olp/core/client/Condition.h
    ./include/olp/core/client/DefaultLookupEndpointProvider.h
    ./include/olp/core/client/ErrorCode.h
    ./include/olp/core/client/FetchOptions.h
    ./include/olp/core/client/HRN.h
    ./include/olp/core/client/HttpResponse.h
    ./include/olp/core/client/OauthToken.h
    ./include/olp/core/client/OlpClient.h
    ./include/olp/core/client/OlpClientFactory.h
    ./include/olp/core/client/OlpClientSettings.h
    ./include/olp/core/client/OlpClientSettingsFactory.h
    ./include/olp/core/client/PendingRequests.h
    ./include/olp/core/client/RetrySettings.h
    ./include/olp/core/client/TaskContext.h
)

set(OLP_SDK_GENERATED_HEADERS
    ./include/olp/core/generated/parser/JsonParser.h
    ./include/olp/core/generated/parser/ParserWrapper.h
    ./include/olp/core/generated/serializer/SerializerWrapper.h
)

set(OLP_SDK_HTTP_HEADERS
    ./include/olp/core/http/HttpStatusCode.h
    ./include/olp/core/http/Network.h
    ./include/olp/core/http/HttpStatusCode.h
    ./include/olp/core/http/NetworkConstants.h
    ./include/olp/core/http/NetworkProxySettings.h
    ./include/olp/core/http/NetworkRequest.h
    ./include/olp/core/http/NetworkResponse.h
    ./include/olp/core/http/NetworkSettings.h
    ./include/olp/core/http/NetworkTypes.h
    ./include/olp/core/http/NetworkUtils.h
)

set(OLP_SDK_MODEL_HEADERS
    ./include/olp/core/client/model/Api.h
)

set(OLP_SDK_PLATFORM_HEADERS
    ./include/olp/core/context/Context.h
)

set(OLP_SDK_PORTING_HEADERS
    ./include/olp/core/porting/deprecated.h
    ./include/olp/core/porting/export.h
    ./include/olp/core/porting/make_unique.h
    ./include/olp/core/porting/platform.h
    ./include/olp/core/porting/shared_mutex.h
    ./include/olp/core/porting/try_emplace.h
    ./include/olp/core/porting/warning_disable.h
)

set(OLP_SDK_UTILS_HEADERS
    ./include/olp/core/utils/Base64.h
    ./include/olp/core/utils/Config.h
    ./include/olp/core/utils/Dir.h
    ./include/olp/core/utils/LruCache.h
    ./include/olp/core/utils/Url.h
    ./include/olp/core/utils/WarningWorkarounds.h
)

set(OLP_SDK_LOGGING_HEADERS
    ./include/olp/core/logging/Appender.h
    ./include/olp/core/logging/BufferAppender.h
    ./include/olp/core/logging/CircularBuffer.h
    ./include/olp/core/logging/Configuration.h
    ./include/olp/core/logging/ConsoleAppender.h
    ./include/olp/core/logging/DebugAppender.h
    ./include/olp/core/logging/FileAppender.h
    ./include/olp/core/logging/FilterGroup.h
    ./include/olp/core/logging/Format.h
    ./include/olp/core/logging/Level.h
    ./include/olp/core/logging/Log.h
    ./include/olp/core/logging/LogMessage.h
    ./include/olp/core/logging/MessageFormatter.h
)

set(OLP_SDK_THREAD_HEADERS
    ./include/olp/core/thread/Atomic.h
    ./include/olp/core/thread/Continuation.h
    ./include/olp/core/thread/Continuation.inl
    ./include/olp/core/thread/ExecutionContext.h
    ./include/olp/core/thread/SyncQueue.h
    ./include/olp/core/thread/SyncQueue.inl
    ./include/olp/core/thread/TaskContinuation.h
    ./include/olp/core/thread/TaskContinuation.inl
    ./include/olp/core/thread/TaskScheduler.h
    ./include/olp/core/thread/ThreadPoolTaskScheduler.h
    ./include/olp/core/thread/TypeHelpers.h
)

set(OLP_SDK_GEOCOORDINATES_HEADERS
    ./include/olp/core/geo/coordinates/GeoCoordinates.h
    ./include/olp/core/geo/coordinates/GeoCoordinates3d.h
    ./include/olp/core/geo/coordinates/GeoPoint.h
    ./include/olp/core/geo/coordinates/GeoRectangle.h
)

set(OLP_SDP_GEOPROJECTION_HEADERS
    ./include/olp/core/geo/projection/EarthConstants.h
    ./include/olp/core/geo/projection/EquirectangularProjection.h
    ./include/olp/core/geo/projection/IdentityProjection.h
    ./include/olp/core/geo/projection/IProjection.h
    ./include/olp/core/geo/projection/SphereProjection.h
    ./include/olp/core/geo/projection/WebMercatorProjection.h
)

set(OLP_SDK_GEOTILING_HEADERS
    ./include/olp/core/geo/tiling/HalfQuadTreeSubdivisionScheme.h
    ./include/olp/core/geo/tiling/ISubdivisionScheme.h
    ./include/olp/core/geo/tiling/ITilingScheme.h
    ./include/olp/core/geo/tiling/QuadTreeSubdivisionScheme.h
    ./include/olp/core/geo/tiling/SubTiles.h
    ./include/olp/core/geo/tiling/TileKey.h
    ./include/olp/core/geo/tiling/TileKeyUtils.h
    ./include/olp/core/geo/tiling/TileTreeTraverse.h
    ./include/olp/core/geo/tiling/TilingScheme.h
    ./include/olp/core/geo/tiling/TilingSchemeRegistry.h
)

set(OLP_SDK_GEOTYPE_HEADER
    ./include/olp/core/geo/Types.h
)

set(OLP_SDK_CONFIG_HEADER
    ${CMAKE_CURRENT_BINARY_DIR}/include/olp/core/Config.h
)

set(OLP_SDK_MATH_HEADERS
    ./include/olp/core/math/AlignedBox.h
    ./include/olp/core/math/Math.h
    ./include/olp/core/math/Size.h
    ./include/olp/core/math/Types.h
    ./include/olp/core/math/Vector.h
)

set(OLP_SDK_COREAPI_HEADERS
    ./include/olp/core/CoreApi.h
)

set(OLP_SDK_GEO_SOURCES
    ./src/geo/coordinates/GeoCoordinates.cpp
    ./src/geo/coordinates/GeoCoordinates3d.cpp
    ./src/geo/coordinates/GeoRectangle.cpp
    ./src/geo/projection/EquirectangularProjection.cpp
    ./src/geo/projection/IdentityProjection.cpp
    ./src/geo/projection/SphereProjection.cpp
    ./src/geo/projection/WebMercatorProjection.cpp
    ./src/geo/tiling/HalfQuadTreeSubdivisionScheme.cpp
    ./src/geo/tiling/QuadTreeSubdivisionScheme.cpp
    ./src/geo/tiling/TileKey.cpp
    ./src/geo/tiling/TileKeyUtils.cpp
    ./src/geo/tiling/TileTreeTraverse.cpp
)

set(OLP_SDK_CACHE_SOURCES
    ./src/cache/DefaultCache.cpp
    ./src/cache/DefaultCacheImpl.cpp
    ./src/cache/DefaultCacheImpl.h
    ./src/cache/DiskCache.cpp
    ./src/cache/DiskCache.h
    ./src/cache/DiskCacheEnv.cpp
    ./src/cache/DiskCacheEnv.h
    ./src/cache/DiskCacheSizeLimitEnv.cpp
    ./src/cache/DiskCacheSizeLimitEnv.h
    ./src/cache/DiskCacheSizeLimitWritableFile.cpp
    ./src/cache/DiskCacheSizeLimitWritableFile.h
    ./src/cache/KeyGenerator.cpp
    ./src/cache/ProtectedKeyList.cpp
    ./src/cache/ProtectedKeyList.h
    ./src/cache/InMemoryCache.cpp
    ./src/cache/InMemoryCache.h
    ./src/cache/ReadOnlyEnv.cpp
    ./src/cache/ReadOnlyEnv.h
)

set(OLP_SDK_CLIENT_SOURCES
    ./src/client/api/PlatformApi.cpp
    ./src/client/api/PlatformApi.h
    ./src/client/api/ResourcesApi.cpp
    ./src/client/api/ResourcesApi.h
    ./src/client/parser/ApiParser.cpp
    ./src/client/parser/ApiParser.h
    ./src/client/repository/ApiCacheRepository.cpp
    ./src/client/repository/ApiCacheRepository.h
    ./src/client/ApiLookupClient.cpp
    ./src/client/ApiLookupClientImpl.cpp
    ./src/client/ApiLookupClientImpl.h
    ./src/client/CancellationToken.cpp
    ./src/client/DefaultLookupEndpointProvider.cpp
    ./src/client/HRN.cpp
    ./src/client/OauthToken.cpp
    ./src/client/OlpClient.cpp
    ./src/client/OlpClientFactory.cpp
    ./src/client/OlpClientSettingsFactory.cpp
    ./src/client/PendingRequests.cpp
    ./src/client/PendingUrlRequests.h
    ./src/client/PendingUrlRequests.cpp
    ./src/client/RetrySettings.cpp
    ./src/client/Tokenizer.h
)

set(OLP_SDK_HTTP_SOURCES
    ./src/http/DefaultNetwork.cpp
    ./src/http/DefaultNetwork.h
    ./src/http/Network.cpp
    ./src/http/NetworkProxySettings.cpp
    ./src/http/NetworkRequest.cpp
    ./src/http/NetworkResponse.cpp
    ./src/http/NetworkSettings.cpp
    ./src/http/NetworkTypes.cpp
    ./src/http/NetworkUtils.cpp
)

if (ANDROID)
    # http network Android implementation:
    set(OLP_SDK_HTTP_ANDROID_SOURCES
        ./src/http/android/NetworkAndroid.h
        ./src/http/android/NetworkAndroid.cpp
        ./src/http/android/utils/JNIThreadBinder.h
        ./src/http/android/utils/JNIScopedLocalReference.h
    )

    set(OLP_SDK_HTTP_SOURCES ${OLP_SDK_HTTP_SOURCES} ${OLP_SDK_HTTP_ANDROID_SOURCES})
    set(OLP_SDK_DEFAULT_NETWORK_DEFINITION OLP_SDK_NETWORK_HAS_ANDROID)
endif()

if (UNIX AND (NOT ANDROID OR OLP_SDK_ENABLE_ANDROID_CURL))
    set(OLP_SDK_HTTP_SOURCES ${OLP_SDK_HTTP_SOURCES} ${OLP_SDK_HTTP_CURL_SOURCES})

    set(OLP_SDK_DEFAULT_NETWORK_DEFINITION OLP_SDK_NETWORK_HAS_CURL)
endif()

if (IOS)
    set(OLP_SDK_HTTP_SOURCES ${OLP_SDK_HTTP_SOURCES} ${OLP_SDK_HTTP_IOS_SOURCES})

    set(OLP_SDK_DEFAULT_NETWORK_DEFINITION OLP_SDK_NETWORK_HAS_IOS)
endif()

if (WIN32)
    if (MINGW)
        set(OLP_SDK_HTTP_SOURCES ${OLP_SDK_HTTP_SOURCES} ${OLP_SDK_HTTP_CURL_SOURCES})
        set(OLP_SDK_DEFAULT_NETWORK_DEFINITION OLP_SDK_NETWORK_HAS_CURL)
    else()
        set(OLP_SDK_HTTP_SOURCES ${OLP_SDK_HTTP_SOURCES} ${OLP_SDK_HTTP_WIN_SOURCES})
        set(OLP_SDK_DEFAULT_NETWORK_DEFINITION OLP_SDK_NETWORK_HAS_WINHTTP)
    endif()
endif()

set(OLP_SDK_PLATFORM_SOURCES
    ./src/context/Context.cpp
)

set(OLP_SDK_UTILS_SOURCES
    ./src/utils/Base64.cpp
    ./src/utils/BoostExceptionHandle.cpp
    ./src/utils/Dir.cpp
    ./src/utils/Url.cpp
)

set(OLP_SDK_LOGGING_SOURCES
    ./src/logging/BufferAppender.cpp
    ./src/logging/Configuration.cpp
    ./src/logging/ConsoleAppender.cpp
    ./src/logging/DebugAppender.cpp
    ./src/logging/FileAppender.cpp
    ./src/logging/FilterGroup.cpp
    ./src/logging/Format.cpp
    ./src/logging/Log.cpp
    ./src/logging/MessageFormatter.cpp
    ./src/logging/ThreadId.cpp
    ./src/logging/ThreadId.h
)

set(OLP_SDK_THREAD_SOURCES
    ./src/thread/Continuation.cpp
    ./src/thread/ExecutionContext.cpp
    ./src/thread/PriorityQueueExtended.h
    ./src/thread/ThreadPoolTaskScheduler.cpp
)

set(OLP_SDK_CORE_HEADERS
    ${OLP_SDK_CACHE_HEADERS}
    ${OLP_SDK_CLIENT_HEADERS}
    ${OLP_SDK_GENERATED_HEADERS}
    ${OLP_SDK_HTTP_HEADERS}
    ${OLP_SDK_MODEL_HEADERS}
    ${OLP_SDK_PLATFORM_HEADERS}
    ${OLP_SDK_PORTING_HEADERS}
    ${OLP_SDK_UTILS_HEADERS}
    ${OLP_SDK_LOGGING_HEADERS}
    ${OLP_SDK_THREAD_HEADERS}
    ${OLP_SDK_MATH_HEADERS}
    ${OLP_SDK_GEOCOORDINATES_HEADERS}
    ${OLP_SDP_GEOPROJECTION_HEADERS}
    ${OLP_SDK_GEOTILING_HEADERS}
    ${OLP_SDK_COREAPI_HEADERS}
    ${OLP_SDK_GEOTYPE_HEADER}
    ${OLP_SDK_CONFIG_HEADER}
)

set(OLP_SDK_CORE_SOURCES
    ${OLP_SDK_CLIENT_SOURCES}
    ${OLP_SDK_HTTP_SOURCES}
    ${OLP_SDK_PLATFORM_SOURCES}
    ${OLP_SDK_UTILS_SOURCES}
    ${OLP_SDK_LOGGING_SOURCES}
    ${OLP_SDK_THREAD_SOURCES}
    ${OLP_SDK_GEO_SOURCES}
)

if(OLP_SDK_ENABLE_DEFAULT_CACHE)
    set(OLP_SDK_CORE_SOURCES ${OLP_SDK_CORE_SOURCES} ${OLP_SDK_CACHE_SOURCES})
endif()

add_library(${PROJECT_NAME} ${OLP_SDK_CORE_SOURCES} ${OLP_SDK_CORE_HEADERS})

if (OLP_SDK_DISABLE_DEBUG_LOGGING)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC LOGGING_DISABLE_DEBUG_LEVEL)
endif()

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

    # Depends on RapidJSON revision, could be one of the following
    $<BUILD_INTERFACE:${RapidJSON_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${RAPIDJSON_INCLUDE_DIRS}>

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
        ${OLP_SDK_NETWORK_CURL_LIBRARIES}
        ${NETWORK_WINHTTP_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
)

if(OLP_SDK_ENABLE_DEFAULT_CACHE)
    target_include_directories(${PROJECT_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${leveldb_INCLUDE_DIR}>
    )
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            leveldb::leveldb
    )
endif()

if(OLP_SDK_ENABLE_DEFAULT_CACHE_LMDB)
    target_include_directories(${PROJECT_NAME}
        PRIVATE
            $<BUILD_INTERFACE:${lmdb_INCLUDE_DIR}>
    )
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            lmdb::lmdb
    )
endif()

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
elseif(ANDROID AND NOT OLP_SDK_ENABLE_ANDROID_CURL)
    # Make sure that OlpHttpClient.jar is built before olp-cpp-sdk-core
    add_dependencies(${PROJECT_NAME} ${OLP_SDK_ANDROID_HTTP_CLIENT_JAR})
endif()

if(CURL_FOUND AND NOT MINGW)
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
install (FILES ${OLP_SDK_MODEL_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/model)
install (FILES ${OLP_SDK_PLATFORM_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/platform)
install (FILES ${OLP_SDK_PORTING_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/porting)
install (FILES ${OLP_SDK_UTILS_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/utils)
install (FILES ${OLP_SDK_THREAD_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/thread)
install (FILES ${OLP_SDK_CACHE_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/cache)
install (FILES ${OLP_SDK_CLIENT_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/client)
install (FILES ${OLP_SDK_MATH_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/math)
install (FILES ${OLP_SDK_GEOCOORDINATES_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/geo/coordinates)
install (FILES ${OLP_SDP_GEOPROJECTION_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/geo/projection)
install (FILES ${OLP_SDK_GEOTILING_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/geo/tiling)
install (FILES ${OLP_SDK_LOGGING_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/logging)
install (FILES ${OLP_SDK_COREAPI_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/core)
install (FILES ${OLP_SDK_GEOTYPE_HEADER} DESTINATION ${INCLUDE_DIRECTORY}/olp/core/geo)
install (FILES ${OLP_SDK_CONFIG_HEADER} DESTINATION ${INCLUDE_DIRECTORY}/olp/core)

export_config()

if(OLP_SDK_ENABLE_TESTING)
    add_subdirectory(tests)
endif()
