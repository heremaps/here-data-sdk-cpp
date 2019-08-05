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
if(IOS)
    aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../src/network/ios NETWORK_IOS_SOURCES)
    ### To show header files in IDEs
    file(GLOB_RECURSE NETWORK_IOS_INTERNAL_INCLUDES "*.h" "*.inl")
    set(NETWORK_IOS_SOURCES
        ${NETWORK_IOS_SOURCES}
        ${NETWORK_IOS_INTERNAL_INCLUDES}
    )

    set(EDGE_SDK_HTTP_IOS_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/ios/OLPNetworkIOS.mm"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/ios/OLPNetworkIOS.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/ios/OLPNetworkConstants.mm"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/ios/OLPNetworkConstants.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/ios/OLPHttpTask.mm"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/ios/OLPHttpTask+Internal.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/ios/OLPHttpTask.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/ios/OLPHttpClient.mm"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/ios/OLPHttpClient+Internal.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/http/ios/OLPHttpClient.h"
    )

    add_definitions(-DNETWORK_HAS_IOS)
else()
    set(NETWORK_IOS_SOURCES)
endif()
