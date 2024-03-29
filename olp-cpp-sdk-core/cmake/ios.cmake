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
    set(OLP_SDK_HTTP_IOS_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/../src/http/ios/OLPNetworkIOS.mm"
        "${CMAKE_CURRENT_LIST_DIR}/../src/http/ios/OLPNetworkIOS.h"
        "${CMAKE_CURRENT_LIST_DIR}/../src/http/ios/OLPNetworkConstants.mm"
        "${CMAKE_CURRENT_LIST_DIR}/../src/http/ios/OLPNetworkConstants.h"
        "${CMAKE_CURRENT_LIST_DIR}/../src/http/ios/OLPHttpTask.mm"
        "${CMAKE_CURRENT_LIST_DIR}/../src/http/ios/OLPHttpTask+Internal.h"
        "${CMAKE_CURRENT_LIST_DIR}/../src/http/ios/OLPHttpTask.h"
        "${CMAKE_CURRENT_LIST_DIR}/../src/http/ios/OLPHttpClient.mm"
        "${CMAKE_CURRENT_LIST_DIR}/../src/http/ios/OLPHttpClient+Internal.h"
        "${CMAKE_CURRENT_LIST_DIR}/../src/http/ios/OLPHttpClient.h"
    )

    set(OLP_SDK_PLATFORM_IOS_HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/../include/olp/core/context/EnterBackgroundSubscriber.h"
    )
    set(OLP_SDK_PLATFORM_IOS_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/../src/context/ios/ContextInternal.mm"
    )

    add_definitions(-DOLP_SDK_NETWORK_HAS_IOS)
else()
    set(OLP_SDK_PLATFORM_IOS_HEADERS)
    set(OLP_SDK_PLATFORM_IOS_SOURCES)
    set(OLP_SDK_HTTP_IOS_SOURCES)
endif()
