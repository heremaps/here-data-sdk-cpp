#!/bin/bash -e
#
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

# This script should run on special docker image with armhf packages installed.
# Compiler setup for ARM
export CC=arm-linux-gnueabihf-gcc-7
export CXX=arm-linux-gnueabihf-g++-7
export LD=arm-linux-gnueabihf-ld

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DOLP_SDK_BUILD_DOC=ON -DBUILD_SHARED_LIBS=ON -OLP_SDK_BUILD_EXAMPLES=OFF -DOLP_SDK_ENABLE_TESTING=OFF ..
make -j$(nproc)
make docs
echo "ARM HF build succeeded."
