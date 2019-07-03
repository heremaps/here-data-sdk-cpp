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

# Build section
set(SDK_LIST "")
list(APPEND SDK_LIST "dataservice-write")
list(APPEND SDK_LIST "authentication")
list(APPEND SDK_LIST "dataservice-read")

set(DATASERVICE-WRITE_DEPENDENCY_LIST "core")
set(AUTHENTICATION_DEPENDENCY_LIST "core")
set(DATASERVICE-READ_DEPENDECY_LIST "core")
set(BUILD_LIST)

function(add_sdk name)
    set(SDK_DIR "olp-cpp-sdk-${name}")
    string(TOUPPER ${name} SDK_NAME)
    set(DEPENDENCY_LIST "${${SDK_NAME}_DEPENDENCY_LIST}")
    foreach(DEPENDENCY IN LISTS DEPENDENCY_LIST)
       add_sdk(${DEPENDENCY})
    endforeach()
    set(BUILD_LIST ${BUILD_LIST} ${SDK_DIR} PARENT_SCOPE)
endfunction()

function(add_sdks)
    if(BUILD_ONLY)
       set(SDK_LIST ${BUILD_ONLY})
    endif()
    foreach(SDK IN LISTS SDK_LIST)
        add_sdk(${SDK})
    endforeach()
    LIST(REMOVE_DUPLICATES BUILD_LIST)
    foreach(SDK_DIR IN LISTS BUILD_LIST)
        add_subdirectory("${SDK_DIR}")
    endforeach()
    set(BUILD_LIST ${BUILD_LIST} PARENT_SCOPE)
endfunction()

# Install section
include(CMakePackageConfigHelpers)
set(INCLUDE_DIRECTORY "include")
set(BINARY_DIRECTORY "bin")
set(LIBRARY_DIRECTORY "lib")
set(ARCHIVE_DIRECTORY "lib")

# Generic description of library dependencies used in package config
# TODO Customize based on platform
set(THREAD_LIBS pthread)
set(NETWORK_LIBS curl crypto ssl z)
set(SDK_LIBS ${THREAD_LIBS}  ${NETWORK_LIBS})
foreach(LIB IN LISTS SDK_LIBS)
    if (SDK_CONFIG_LIBS)
        set(SDK_CONFIG_LIBS "${SDK_CONFIG_LIBS} -l${LIB}")
    else()
        set(SDK_CONFIG_LIBS "-l${LIB}")
    endif()
endforeach()

macro(export_config)
    # Install and export targets
    install (
        TARGETS ${PROJECT_NAME}
        EXPORT "${PROJECT_NAME}-targets"
        ARCHIVE DESTINATION ${ARCHIVE_DIRECTORY}
        LIBRARY DESTINATION ${LIBRARY_DIRECTORY}
        RUNTIME DESTINATION ${BINARY_DIRECTORY}
    )
    export(EXPORT "${PROJECT_NAME}-targets"
        FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-targets.cmake"
    )
    set(ConfigPackageLocation "${LIBRARY_DIRECTORY}/cmake/${PROJECT_NAME}")
    install(EXPORT "${PROJECT_NAME}-targets"
        FILE "${PROJECT_NAME}-targets.cmake"
        DESTINATION ${ConfigPackageLocation}
    )

    # Create and install configuration files
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )
    if(${PROJECT_NAME} STREQUAL "olp-cpp-sdk-core")
        configure_file(
            "${OLP_CPP_SDK_ROOT}/cmake/core-config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
            @ONLY)
    else()
         configure_file(
             "${OLP_CPP_SDK_ROOT}/cmake/cmakeProjectConfig.cmake"
             "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
             @ONLY)
    endif()
    install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
        DESTINATION
        ${ConfigPackageLocation}
        COMPONENT
        Devel)
    if (BUILD_SHARED_LIBS)
        configure_file("${OLP_CPP_SDK_ROOT}/cmake/pkg-config.pc.in" "${PROJECT_NAME}.pc" @ONLY)
        install(
            FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
            DESTINATION ${LIBRARY_DIRECTORY}/pkgconfig)
    endif()
endmacro()

