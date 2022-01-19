# Copyright (C) 2019-2020 HERE Europe B.V.
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

set(OLP_SDK_DATASERVICE_EXAMPLE_TARGET dataservice-example)
set(OLP_SDK_DATASERVICE_READ_EXAMPLE_TARGET dataservice-read-example)
set(OLP_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET dataservice-write-example)
set(OLP_SDK_DATASERVICE_CACHE_EXAMPLE_TARGET dataservice-cache-example)
set(OLP_SDK_DATASERVICE_READ_STREAM_LAYER_EXAMPLE_TARGET dataservice-read-stream-layer-example)

set(OLP_SDK_EXAMPLE_SUCCESS_STRING "Example has finished successfully")
set(OLP_SDK_EXAMPLE_FAILURE_STRING "Example failed!")
if (IOS)
    include(./cmake/gen_ios_example.cmake.in)


    gen_ios_example_application(${OLP_SDK_DATASERVICE_EXAMPLE_TARGET}
                                ${OLP_SDK_EXAMPLE_SUCCESS_STRING}
                                ${OLP_SDK_EXAMPLE_FAILURE_STRING})
elseif(ANDROID)
    include(./cmake/gen_android_example.cmake.in)

    set(OLP_SDK_EXAMPLE_PACKAGE_NAME "dataservice_example")
    # If JNI function contains underscore symbol, java will search for "_1" instead
    set(OLP_SDK_EXAMPLE_PACKAGE_JNI_NAME "dataservice_1example")
    gen_android_example_application(${OLP_SDK_DATASERVICE_EXAMPLE_TARGET}
                                    ${OLP_SDK_EXAMPLE_PACKAGE_NAME}
                                    ${OLP_SDK_EXAMPLE_PACKAGE_JNI_NAME}
                                    ${OLP_SDK_EXAMPLE_SUCCESS_STRING}
                                    ${OLP_SDK_EXAMPLE_FAILURE_STRING})

else()
    project(${OLP_SDK_DATASERVICE_EXAMPLE_TARGET})

    add_library(${OLP_SDK_DATASERVICE_READ_EXAMPLE_TARGET}
        ./ReadExample.cpp
        ./Examples.h
        ./ReadExample.h)

    target_link_libraries(${OLP_SDK_DATASERVICE_READ_EXAMPLE_TARGET}
        olp-cpp-sdk-authentication
        olp-cpp-sdk-dataservice-read)
        
    add_library(${OLP_SDK_DATASERVICE_READ_STREAM_LAYER_EXAMPLE_TARGET}
        ./StreamLayerReadExample.cpp
        ./Examples.h
        ./StreamLayerReadExample.h)

    target_link_libraries(${OLP_SDK_DATASERVICE_READ_STREAM_LAYER_EXAMPLE_TARGET}
        olp-cpp-sdk-authentication
        olp-cpp-sdk-dataservice-read)

    add_library(${OLP_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET}
        ./WriteExample.cpp
        ./Examples.h
        ./WriteExample.h)

    target_link_libraries(${OLP_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET}
        olp-cpp-sdk-authentication
        olp-cpp-sdk-dataservice-write)

    add_library(${OLP_SDK_DATASERVICE_CACHE_EXAMPLE_TARGET}
        ./ProtectedCacheExample.cpp
        ./Examples.h
        ./ProtectedCacheExample.h)

    target_link_libraries(${OLP_SDK_DATASERVICE_CACHE_EXAMPLE_TARGET}
        olp-cpp-sdk-authentication
        olp-cpp-sdk-dataservice-read)

    add_executable(${OLP_SDK_DATASERVICE_EXAMPLE_TARGET}
        ./main.cpp
        ./Examples.h
        ./Options.h)

    target_link_libraries(${OLP_SDK_DATASERVICE_EXAMPLE_TARGET}
       ${OLP_SDK_DATASERVICE_READ_EXAMPLE_TARGET}
       ${OLP_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET}
       ${OLP_SDK_DATASERVICE_CACHE_EXAMPLE_TARGET}
       ${OLP_SDK_DATASERVICE_READ_STREAM_LAYER_EXAMPLE_TARGET})

endif()
