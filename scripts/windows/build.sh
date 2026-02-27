#!/bin/bash -e
#
# Copyright (C) 2019-2024 HERE Europe B.V.
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
# Build HERE Data SDK for C++ on Windows using CMake.
#

readonly BUILD_DIR="build"

echo "=== Preparing build directory ==="

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "=== Configuring project ==="

cmake .. \
  -G "${GENERATOR}" \
  -A x64 \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "=== Building project ==="
cmake --build . --config "${BUILD_TYPE}"

echo "Windows build completed successfully."
