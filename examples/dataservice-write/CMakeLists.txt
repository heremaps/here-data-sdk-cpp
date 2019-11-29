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

set(OLP_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET dataservice-write-example)
set(OLP_SDK_EXAMPLE_SUCCESS_STRING "Publish Successfull")
set(OLP_SDK_EXAMPLE_FAILURE_STRING "Publish Failed!")

if (IOS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../helpers/cmake/gen_ios_example.cmake.in)


    gen_ios_example_application(${OLP_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET}
                                ${OLP_SDK_EXAMPLE_SUCCESS_STRING}
                                ${OLP_SDK_EXAMPLE_FAILURE_STRING})
elseif(ANDROID)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../helpers/cmake/gen_android_example.cmake.in)

    set(OLP_SDK_EXAMPLE_PACKAGE_NAME "dataservice_write_example")
    set(OLP_SDK_EXAMPLE_PACKAGE_JNI_NAME "dataservice_1write_1example")
    gen_android_example_application(${OLP_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET}
                                    ${OLP_SDK_EXAMPLE_PACKAGE_NAME}
                                    ${OLP_SDK_EXAMPLE_PACKAGE_JNI_NAME}
                                    ${OLP_SDK_EXAMPLE_SUCCESS_STRING}
                                    ${OLP_SDK_EXAMPLE_FAILURE_STRING})

else()
    project(${OLP_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET} VERSION 1.0.0)

    add_executable(${OLP_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/example.h ${CMAKE_CURRENT_SOURCE_DIR}/example.cpp)

    target_sources(${OLP_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

    target_link_libraries(${OLP_SDK_DATASERVICE_WRITE_EXAMPLE_TARGET}
        olp-cpp-sdk-authentication
        olp-cpp-sdk-dataservice-write
    )
endif()