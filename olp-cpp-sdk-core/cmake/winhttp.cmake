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
if(WIN32 AND NOT MINGW)
    set(OLP_SDK_HTTP_WIN_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/../src/http/winhttp/NetworkWinHttp.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/../src/http/winhttp/NetworkWinHttp.h"
    )

    add_definitions(-DOLP_SDK_NETWORK_HAS_WINHTTP)
    set(NETWORK_WINHTTP_LIBRARIES winhttp)

    find_package(ZLIB QUIET)
    if(ZLIB_FOUND)
        add_definitions(-DNETWORK_HAS_ZLIB)
        include_directories(${ZLIB_INCLUDE_DIR})
        set(NETWORK_WINHTTP_LIBRARIES ${NETWORK_WINHTTP_LIBRARIES} ${ZLIB_LIBRARIES})
    endif()
    set(NETWORK_WINHTTP_LIBRARIES ${NETWORK_WINHTTP_LIBRARIES} Ws2_32)
else()
    set(OLP_SDK_HTTP_WIN_SOURCES)
    set(NETWORK_WINHTTP_LIBRARIES)
endif()
