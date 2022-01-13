#!/bin/bash -e
#
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

# Due to some bug which is cmake cannot detect compiler while called
# from cmake itself when project is compiled with XCode 12.4 we must
# switch to old XCode as a workaround.
sudo xcode-select -s /Applications/Xcode_11.7.app

mkdir -p build && cd build
cmake ../ -GXcode \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/iOS.cmake \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DPLATFORM=iphoneos \
    -DOLP_SDK_ENABLE_TESTING=ON \
    -DSIMULATOR=YES \
    -DOLP_SDK_BUILD_EXAMPLES=ON

xcodebuild
