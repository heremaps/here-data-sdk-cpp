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

# This file is based off of the Platform/Darwin.cmake and Platform/UnixPaths.cmake
# files which are included with CMake 2.8.4
# It has been altered for Apple *OS development
# Initial source: https://code.google.com/p/ios-cmake/

# Options:
#
# PLATFORM
#   This decides which SDK will be selected. Possible values:
#     * ios          - Apple iPhone / iPad / iPod Touch SDK will be selected;
#     * appletvos    - Apple TV SDK will be selected;
#     * applewatchos - Apple Watch SDK will be selected;
#   By default ios is selected.
#
# SIMULATOR
#   This forces SDKS will be selected from the <Platform>Simulator.platform folder,
#   if omitted <Platform>OS.platform is used.
#
# CMAKE_APPLE_PLATFORM_DEVELOPER_ROOT = automatic(default) or /path/to/platform/Developer folder
#   By default this location is automatcially chosen based on the PLATFORM value above.
#   If set manually, it will override the default location and force the user of a particular Developer Platform
#
# CMAKE_APPLE_SDK_ROOT = automatic(default) or /path/to/platform/Developer/SDKs/SDK folder
#   By default this location is automatcially chosen based on the CMAKE_APPLE_PLATFORM_DEVELOPER_ROOT value.
#   In this case it will always be the most up-to-date SDK found in the CMAKE_APPLE_PLATFORM_DEVELOPER_ROOT path.
#   If set manually, this will force the use of a specific SDK version
#
#

# Macros:
#
# set_xcode_property (TARGET XCODE_PROPERTY XCODE_VALUE)
#  A convenience macro for setting xcode specific properties on targets
#  example: set_xcode_property (myioslib IPHONEOS_DEPLOYMENT_TARGET "3.1")
#
# find_host_package (PROGRAM ARGS)
#  A macro used to find executable programs on the host system, not within the Apple *OS environment.
#  Thanks to the android-cmake project for providing the command

# Fix for PThread library not in path
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)


# Standard settings
set (CMAKE_SYSTEM_NAME Darwin)
set (UNIX True)
set (APPLE True)
set (IOS True)

# Hotfix in case the toolchain is executed twice, it will happend everytime "try_compile()"
# Default value iphoneos.
if("$ENV{xcode_platform}" STREQUAL "")
    if (PLATFORM STREQUAL "" OR NOT PLATFORM)
        set(ENV{xcode_platform} "iphoneos")
    else()
        set(ENV{xcode_platform} ${PLATFORM})
    endif()
endif()

set(XCODE_PLATFORM $ENV{xcode_platform})

if (XCODE_PLATFORM STREQUAL "iphoneos")
    set (APPLE_IOS True)
elseif (XCODE_PLATFORM STREQUAL "tvos")
    set (APPLE_TV True)
elseif (XCODE_PLATFORM STREQUAL "watchos")
    set (APPLE_WATCH True)
endif ()

# Set minimum CMake version
if (XCODE_VERSION VERSION_EQUAL 10 OR XCODE_VERSION VERSION_GREATER 10)
    message(STATUS "XCODE_VERSION ${XCODE_VERSION}")
    cmake_minimum_required(VERSION 3.12.1)
endif()

# Required as of cmake 2.8.10
set (CMAKE_OSX_DEPLOYMENT_TARGET "" CACHE STRING "Force unset of the deployment target for Apple *OS" FORCE)

function(get_sdk_path platform type_of_path output_path)
    set(_env_cache_lookup_key "_get_sdk_path_${platform}_${type_of_path}")
    set(_env_cache_lookup $ENV{${_env_cache_lookup_key}})
    if("${_env_cache_lookup}" STREQUAL "" OR "${_cc_path}" STREQUAL "")
        if("${type_of_path}" STREQUAL "platform")
            set(_cmd xcrun --sdk "${platform}" --show-sdk-platform-path)
        elseif("${type_of_path}" STREQUAL "developer")
            set(_cmd xcode-select -print-path)
        elseif("${type_of_path}" STREQUAL "sdk")
            set(_cmd xcrun --sdk "${platform}" --show-sdk-path)
        else()
            message(FATAL "Please set what type of path to look up.")
        endif()
        execute_process(
            COMMAND ${_cmd}
            RESULT_VARIABLE _result
            OUTPUT_VARIABLE _output_path
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(ENV{${_env_cache_lookup_key}} ${_output_path})
    else()
        set(_output_path ${_env_cache_lookup})
    endif()
    set(${output_path} ${_output_path} PARENT_SCOPE)
endfunction()

set(_c_path $ENV{ios_clang_c_path})
set(_cc_path $ENV{ios_clang_cc_path})
if("${_c_path}" STREQUAL "" OR "${_cc_path}" STREQUAL "")
    function(find_using_xcrun command_to_find output_path)
        set(_cmd xcrun --find "${command_to_find}")
        execute_process(
            COMMAND
            ${_cmd}
            OUTPUT_VARIABLE _output_path
            RESULT_VARIABLE _result
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(NOT _result EQUAL 0)
            message(FATAL_ERROR "Command failed: ${_cmd}")
        endif()
        set(${output_path} ${_output_path} PARENT_SCOPE)
    endfunction(find_using_xcrun)
    find_using_xcrun("clang" _c_path)
    find_using_xcrun("clang++" _cc_path)
    set(ENV{ios_clang_c_path} ${_c_path})
    set(ENV{ios_clang_cc_path} ${_cc_path})
endif()

set(CMAKE_C_COMPILER "${_c_path}")
set(CMAKE_C_COMPILER_ID AppleClang)
set(CMAKE_XCODE_ATTRIBUTE_CC "${_c_path}")
set(CMAKE_CXX_COMPILER "${_cc_path}")
set(CMAKE_CXX_COMPILER_ID AppleClang)

# All Apple *OS/Darwin specific settings - some may be redundant
set (CMAKE_SHARED_LIBRARY_PREFIX "lib")
set (CMAKE_SHARED_LIBRARY_SUFFIX ".dylib")
set (CMAKE_SHARED_MODULE_PREFIX "lib")
set (CMAKE_SHARED_MODULE_SUFFIX ".so")
set (CMAKE_MODULE_EXISTS 1)
set (CMAKE_DL_LIBS "")

set (CMAKE_C_OSX_COMPATIBILITY_VERSION_FLAG "-compatibility_version ")
set (CMAKE_C_OSX_CURRENT_VERSION_FLAG "-current_version ")
set (CMAKE_CXX_OSX_COMPATIBILITY_VERSION_FLAG "${CMAKE_C_OSX_COMPATIBILITY_VERSION_FLAG}")
set (CMAKE_CXX_OSX_CURRENT_VERSION_FLAG "${CMAKE_C_OSX_CURRENT_VERSION_FLAG}")

# hack: if a new cmake (which uses CMAKE_INSTALL_NAME_TOOL) runs on an old build tree
# (where install_name_tool was hardcoded) and where CMAKE_INSTALL_NAME_TOOL isn't in the cache
# and still cmake didn't fail in CMakeFindBinUtils.cmake (because it isn't rerun)
# hardcode CMAKE_INSTALL_NAME_TOOL here to install_name_tool, so it behaves as it did before, Alex
if (NOT DEFINED CMAKE_INSTALL_NAME_TOOL)
    find_program(CMAKE_INSTALL_NAME_TOOL install_name_tool)
endif (NOT DEFINED CMAKE_INSTALL_NAME_TOOL)

# Check the platform selection and setup for developer root
if (APPLE_IOS)
    set (XCODE_PLATFORM_SUFFIX "ios")
    if (SIMULATOR)
        set (APPLE_PLATFORM_LOCATION "iPhoneSimulator.platform")
        set (XCODE_IOS_PLATFORM "iphonesimulator")
        # This causes the installers to properly locate the output libraries
        set (CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphonesimulator")
    else ()
        set (APPLE_PLATFORM_LOCATION "iPhoneOS.platform")
        set (XCODE_IOS_PLATFORM "iphoneos")
        # This causes the installers to properly locate the output libraries
        set (CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos")
    endif ()
elseif (APPLE_WATCH)
    set (XCODE_PLATFORM_SUFFIX "watchos")
    if (SIMULATOR)
        set (APPLE_PLATFORM_LOCATION "WatchSimulator.platform")
        set (XCODE_IOS_PLATFORM "watchsimulator")
        # This causes the installers to properly locate the output libraries
        set (CMAKE_XCODE_EFFECTIVE_PLATFORMS "-watchsimulator")
    else ()
        set (APPLE_PLATFORM_LOCATION "WatchOS.platform")
        set (XCODE_IOS_PLATFORM "watchos")
        # This causes the installers to properly locate the output libraries
        set (CMAKE_XCODE_EFFECTIVE_PLATFORMS "-watchos")
    endif ()
elseif (APPLE_TV)
    set (XCODE_PLATFORM_SUFFIX "tvos")
    if (SIMULATOR)
        set (APPLE_PLATFORM_LOCATION "AppleTVSimulator.platform")
        set (XCODE_IOS_PLATFORM "appletvsimulator")
        # This causes the installers to properly locate the output libraries
        set (CMAKE_XCODE_EFFECTIVE_PLATFORMS "-appletvsimulator")
    else ()
        set (APPLE_PLATFORM_LOCATION "AppleTVOS.platform")
        set (XCODE_IOS_PLATFORM "appletvos")
        # This causes the installers to properly locate the output libraries
        set (CMAKE_XCODE_EFFECTIVE_PLATFORMS "-appletvos")
    endif ()
else ()
    message (FATAL_ERROR "Unsupported PLATFORM value selected: ${XCODE_PLATFORM}. ${APPLE_IOS}")
endif ()

get_sdk_path(${XCODE_IOS_PLATFORM} "sdk" CMAKE_APPLE_SDK_ROOT)
get_sdk_path(${XCODE_IOS_PLATFORM} "platform" CMAKE_APPLE_PLATFORM_DEVELOPER_ROOT)
get_sdk_path(${XCODE_IOS_PLATFORM} "developer" CMAKE_APPLE_DEVELOPER_ROOT)

# Set the sysroot default to the most recent SDK
set (CMAKE_OSX_SYSROOT ${CMAKE_APPLE_SDK_ROOT} CACHE PATH "Sysroot used for Apple *OS support")

# set the architecture for Apple *OS
set (VERACODE $ENV{BUILD_VERACODE})

if (SIMULATOR)
    if(APPLE_WATCH OR VERACODE)
        set(APPLE_ARCH i386)
    else()
        set(APPLE_ARCH x86_64)
    endif()
else ()
    if (APPLE_IOS)
        if (VERACODE)
            set (APPLE_ARCH armv7)
        else ()
            set (APPLE_ARCH arm64)
        endif ()
    elseif (APPLE_WATCH)
        set (APPLE_ARCH armv7k)
    elseif (APPLE_TV)
        set (APPLE_ARCH arm64)
    endif ()
endif ()
set (CMAKE_OSX_ARCHITECTURES ${APPLE_ARCH} CACHE STRING  "Build architecture for Apple *OS")

# Set IOS Min Version
if (VERACODE)
    set (APPLE_IOS_VERSION_MIN "10.0" CACHE STRING "Minimum supported Apple iOS version.")
else ()
    set (APPLE_IOS_VERSION_MIN "11.0" CACHE STRING "Minimum supported Apple iOS version.")
endif ()
set (APPLE_WATCH_VERSION_MIN "2.0" CACHE STRING "Minimum supported Apple Watch version.")
set (APPLE_TV_VERSION_MIN "11.0" CACHE STRING "Minimum supported Apple TV version.")

if (APPLE_IOS)
    if (SIMULATOR)
        set (APPLE_VERSION_FLAG "-mios-simulator-version-min=${APPLE_IOS_VERSION_MIN}")
    else ()
        set (APPLE_VERSION_FLAG "-miphoneos-version-min=${APPLE_IOS_VERSION_MIN}")
    endif ()
    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${APPLE_IOS_VERSION_MIN})
    set (APPLE_PLATFORM_VERSION_MIN ${APPLE_IOS_VERSION_MIN})
elseif (APPLE_WATCH)
    if (SIMULATOR)
        set (APPLE_VERSION_FLAG "-mwatchos-simulator-version-min=${APPLE_WATCH_VERSION_MIN}")
    else ()
        set (APPLE_VERSION_FLAG "-mwatchos-version-min=${APPLE_WATCH_VERSION_MIN}")
    endif ()
    set(CMAKE_XCODE_ATTRIBUTE_WATCHOS_DEPLOYMENT_TARGET ${APPLE_WATCH_VERSION_MIN})
    set (APPLE_PLATFORM_VERSION_MIN ${APPLE_WATCH_VERSION_MIN})
elseif (APPLE_TV)
    if (SIMULATOR)
        set (APPLE_VERSION_FLAG "-mtvos-simulator-version-min=${APPLE_TV_VERSION_MIN}")
    else ()
        set (APPLE_VERSION_FLAG "-mtvos-version-min=${APPLE_TV_VERSION_MIN}")
    endif ()
    set(CMAKE_XCODE_ATTRIBUTE_TVOS_DEPLOYMENT_TARGET ${APPLE_TV_VERSION_MIN})
    set (APPLE_PLATFORM_VERSION_MIN ${APPLE_TV_VERSION_MIN})
endif ()
set (IOS_DEPLOYMENT_TARGET ${APPLE_PLATFORM_VERSION_MIN} CACHE STRING "Minimum version of the target platform")

# System version should match with the minimum deployment target.
# Otherwise it could affect INSTALL_NAME_DIR and INSTALL_RPATH cmake properties.
set (CMAKE_SYSTEM_VERSION ${IOS_DEPLOYMENT_TARGET})

# Set the find root to the Apple *OS developer roots and to user defined paths
set (CMAKE_FIND_ROOT_PATH
    ${CMAKE_APPLE_DEVELOPER_ROOT}
    ${CMAKE_APPLE_PLATFORM_DEVELOPER_ROOT}
    ${CMAKE_APPLE_SDK_ROOT}
    ${CMAKE_PREFIX_PATH}
    CACHE STRING "Apple *OS find search path root"
)

# default to searching for frameworks first
set (CMAKE_FIND_FRAMEWORK FIRST)

# set up the default search directories for frameworks
set (CMAKE_SYSTEM_FRAMEWORK_PATH
    ${CMAKE_APPLE_SDK_ROOT}/System/Library/Frameworks
    ${CMAKE_APPLE_SDK_ROOT}/System/Library/PrivateFrameworks
    ${CMAKE_APPLE_SDK_ROOT}/Developer/Library/Frameworks
)

# only search the Apple *OS sdks, not the remainder of the host filesystem
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Symbols visibilty
set (CMAKE_C_FLAGS "${APPLE_VERSION_FLAG} -isysroot ${CMAKE_OSX_SYSROOT} -fvisibility=hidden -fvisibility-inlines-hidden -Wno-deprecated")
set (CMAKE_CXX_FLAGS "${APPLE_VERSION_FLAG} -isysroot ${CMAKE_OSX_SYSROOT} -fvisibility=hidden -fvisibility-inlines-hidden -Wno-deprecated")

set (CMAKE_C_LINK_FLAGS "${APPLE_VERSION_FLAG} -Wl,-search_paths_first ${CMAKE_C_LINK_FLAGS}")
set (CMAKE_CXX_LINK_FLAGS "${APPLE_VERSION_FLAG} -Wl,-search_paths_first ${CMAKE_CXX_LINK_FLAGS}")

# Change the type of target generated for try_compile() so it'll work when cross-compiling
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# In order to ensure that the updated compiler flags are used in try_compile()
# tests, we have to forcibly set them in the CMake cache, not merely set them
# in the local scope.
list(APPEND VARS_TO_FORCE_IN_CACHE
  CMAKE_C_FLAGS
  CMAKE_CXX_FLAGS
  CMAKE_CXX_RELEASE
  CMAKE_C_LINK_FLAGS
  CMAKE_CXX_LINK_FLAGS)
foreach(VAR_TO_FORCE ${VARS_TO_FORCE_IN_CACHE})
  set(${VAR_TO_FORCE} "${${VAR_TO_FORCE}}" CACHE STRING "" FORCE)
endforeach()

set (CMAKE_PLATFORM_HAS_INSTALLNAME 1)
set (CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-dynamiclib -headerpad_max_install_names")
set (CMAKE_SHARED_MODULE_CREATE_C_FLAGS "-bundle -headerpad_max_install_names")
set (CMAKE_SHARED_MODULE_LOADER_C_FLAG "-Wl,-bundle_loader,")
set (CMAKE_SHARED_MODULE_LOADER_CXX_FLAG "-Wl,-bundle_loader,")
set (CMAKE_FIND_LIBRARY_SUFFIXES ".tbd" ".dylib" ".so" ".a")

string (REPLACE ";" "-" PLATFORM_ARCH "${APPLE_ARCH}")
set (PLATFORM_ARCH ${PLATFORM_ARCH} CACHE STRING "Target processor architecture")

# required configuration for https://public.kitware.com/Bug/view.php?id=15329
set(CMAKE_MACOSX_BUNDLE YES)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE YES)

# This little macro lets you set any XCode specific property
macro (set_xcode_property TARGET XCODE_PROPERTY XCODE_VALUE)
    set_property (TARGET ${TARGET} PROPERTY XCODE_ATTRIBUTE_${XCODE_PROPERTY} ${XCODE_VALUE})
endmacro (set_xcode_property)

# This macro lets you find executable programs on the host system
macro (find_host_package)
    set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
    set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
    set (IOS FALSE)

    find_package(${ARGN})

    set (IOS TRUE)
    set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
    set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endmacro (find_host_package)
