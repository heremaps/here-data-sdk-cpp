#!/bin/bash -ex
#
# Copyright (C) 2019-2026 HERE Europe B.V.
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

#
# This script will compile Data SDK for C++
# with ANDROID_PLATFORM=android-21 and -DANDROID_ABI=arm64-v8a
# by using Android NDK 21.
#

#!/usr/bin/env bash
set -euo pipefail

#
# Build HERE Data SDK for C++ for Android.
#
# Configuration:
# - Android platform: android-21
# - ABI: arm64-v8a
# - NDK version: 21.3.6528147
#

readonly NDK_VERSION="21.3.6528147"
readonly ANDROID_PLATFORM="android-21"
readonly ANDROID_ABI="arm64-v8a"
readonly BUILD_DIR="build"

echo "=== Installing Android dependencies ==="

"${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager" --install \
  "ndk;${NDK_VERSION}" \
  "platforms;${ANDROID_PLATFORM}" \
  --sdk_root="${ANDROID_HOME}" >/dev/null

export ANDROID_NDK_HOME="${ANDROID_HOME}/ndk/${NDK_VERSION}"

echo "NDK installed at: ${ANDROID_NDK_HOME}"
ls -la "${ANDROID_NDK_HOME}"

echo "=== Configuring project ==="

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DANDROID_PLATFORM="${ANDROID_PLATFORM}" \
  -DANDROID_ABI="${ANDROID_ABI}" \
  -DANDROID_NDK="${ANDROID_NDK_HOME}" \
  -DOLP_SDK_ENABLE_TESTING=OFF \
  -DOLP_SDK_BUILD_EXAMPLES=ON

echo "=== Building project ==="
sudo make install -j"$(nproc)"

echo "=== Building Android examples ==="
pushd ../examples/android > /dev/null
echo "Java version: $(java --version)"
./gradlew assemble --stacktrace
popd > /dev/null

echo "Android build completed successfully."
