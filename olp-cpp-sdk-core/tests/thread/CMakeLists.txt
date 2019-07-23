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

file(GLOB_RECURSE EDGE_SDK_SOURCES "unittest/*.cpp" "unittest/*.h")

set(EDGE_SDK_TARGET thread)

add_executable(${EDGE_SDK_TARGET}-test ${EDGE_SDK_SOURCES})

target_include_directories(${EDGE_SDK_TARGET}-test
    PRIVATE ${PROJECT_SOURCE_DIR}/tests
)

target_link_libraries(${EDGE_SDK_TARGET}-test
   PRIVATE olp-cpp-sdk-core
   PRIVATE gtest_main
   PRIVATE gmock_main
)

