#!/bin/bash -e
#
# Copyright (C) 2019-2025 HERE Europe B.V.
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
#!/usr/bin/env bash
set -euo pipefail

#
# Cross-compile HERE Data SDK for C++ for ARM (armhf).
# Requires Docker image with ARM toolchain installed.
#

readonly BUILD_DIR="build"

echo "=== Configuring ARM toolchain ==="

export CC=arm-linux-gnueabihf-gcc-7
export CXX=arm-linux-gnueabihf-g++-7
export LD=arm-linux-gnueabihf-ld

echo "=== Configuring project ==="

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake .. \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_SHARED_LIBS=ON \
  -DOLP_SDK_BUILD_EXAMPLES=OFF \
  -DOLP_SDK_ENABLE_TESTING=OFF

echo "=== Building project ==="
make -j"$(nproc)"

echo "ARM HF build completed successfully."
