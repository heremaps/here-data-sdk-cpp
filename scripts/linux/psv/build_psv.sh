#!/bin/bash -ex
#
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

# Show initial ccache data
ccache -s

# For core dump backtrace
ulimit -c unlimited

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_CXX_FLAGS="-Wall -Wextra -Werror" \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DOLP_SDK_BUILD_EXAMPLES=ON \
    -DBUILD_SHARED_LIBS=ON \
    ..

make -j$(nproc)

# Show last ccache data
ccache -s
