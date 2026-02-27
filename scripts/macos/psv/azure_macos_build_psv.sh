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


#!/usr/bin/env bash
set -euo pipefail

#
# Build HERE Data SDK for C++ on macOS.
#

readonly BUILD_DIR="build"

echo "=== Configuring macOS build ==="

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake .. \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DBUILD_SHARED_LIBS=ON \
  -DOLP_SDK_ENABLE_TESTING=OFF \
  -DOLP_SDK_BUILD_EXAMPLES=ON

echo "=== Building project ==="
make -j"$(sysctl -n hw.ncpu)"

echo "macOS build completed successfully."
