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


mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
	-DANDROID_PLATFORM=android-28 \
	-DANDROID_ABI=arm64-v8a \
	-DOLP_SDK_ENABLE_TESTING=NO \
	-DOLP_SDK_BUILD_EXAMPLES=ON

#cmake --build . # this is alternative option for build
sudo make install -j
cd $WORKSPACE/build/examples/dataservice-read/android
sudo ./gradlew assemble
cd $WORKSPACE/build/examples/dataservice-write/android
sudo ./gradlew assemble
