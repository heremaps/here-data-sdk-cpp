#!/bin/bash -ex
#
# Copyright (C) 2026 HERE Europe B.V.
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

BUILD_DIR="build-clang-tidy"

rm -rf "${BUILD_DIR}"
mkdir "${BUILD_DIR}"

cmake -S . -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_CXX_COMPILER=clang++-17 \
    -DCMAKE_C_COMPILER=clang-17 \
    -DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations" \
    -DOLP_SDK_ENABLE_TESTING=OFF \
    -DOLP_SDK_BUILD_EXAMPLES=OFF

cmake --build "${BUILD_DIR}" -- -j"$(nproc)"

run-clang-tidy-17.py -p "${BUILD_DIR}" \
    -config-file="${PWD}/.clang-tidy" \
    "${PWD}/olp-cpp-sdk-core/.*" \
    "${PWD}/olp-cpp-sdk-authentication/.*" \
    "${PWD}/olp-cpp-sdk-dataservice-read/.*" \
    "${PWD}/olp-cpp-sdk-dataservice-write/.*"