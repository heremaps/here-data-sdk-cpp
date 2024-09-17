#!/bin/bash -ex
#
# Copyright (C) 2020 HERE Europe B.V.
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

# This is script do:
# 1. Build sdk from master
# 2. Build sdk from previous tagged version
# 3. Generate abi and api information for both versions, by using tool from https://github.com/lvc/abi-dumper
# 4. Generate report with diff in html for every lib, by using tool from https://github.com/lvc/abi-compliance-checker

# Set workspace location
if [[ ${CI_PROJECT_DIR} == "" ]]; then
    export CI_PROJECT_DIR=`pwd`
fi

export PREVIOUS_SDK_VERSION="$( git describe --tags --abbrev=0 )"
export PREVIOUS_SDK_VERSION_NUM="$( git describe --tags --abbrev=0 |  cut -f2 -d'v' )"
export CURRENT_SDK_VERSION="master"

# Debug mode needed for api compliance tools
# CXXFLAGS is needed for hiding internal API data
export BUILD_TYPE="Debug"
export CXXFLAGS="-fvisibility=hidden"

# For core dump backtrace
ulimit -c unlimited

# Building latest master SDK
mkdir -p build
pushd build
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DBUILD_SHARED_LIBS=ON \
        -DOLP_SDK_ENABLE_TESTING=NO \
        ..
    make -j$(nproc)
popd

# Generating dump for CURRENT_SDK_VERSION lib for API diff
for lib_name in olp-cpp-sdk-authentication olp-cpp-sdk-core olp-cpp-sdk-dataservice-read olp-cpp-sdk-dataservice-write
do
    pushd build/${lib_name}
        abi-dumper lib${lib_name}.so -o /workspace/ABIv1${lib_name}.dump -lambda
    popd
done
mv -f build build_${CURRENT_SDK_VERSION}

# Building previous version SDK
git checkout tags/${PREVIOUS_SDK_VERSION}
mkdir -p build
pushd build
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DBUILD_SHARED_LIBS=ON \
        -DOLP_SDK_ENABLE_TESTING=NO \
        ..
    make -j$(nproc)
popd

# Generating dump for PREVIOUS_SDK_VERSION_NUM lib for API diff
for lib_name in olp-cpp-sdk-authentication olp-cpp-sdk-core olp-cpp-sdk-dataservice-read olp-cpp-sdk-dataservice-write
do
    pushd build/${lib_name}
        abi-dumper lib${lib_name}.so -o /workspace/ABIv0${lib_name}.dump -vnum ${PREVIOUS_SDK_VERSION_NUM} -lambda
    popd
    set +e
    abi-compliance-checker -l lib${lib_name} -old /workspace/ABIv0${lib_name}.dump -new /workspace/ABIv1${lib_name}.dump
    echo "Exit code is $? , see details : https://lvc.github.io/abi-compliance-checker/#Error "
    set -e
    cp -f "${CI_PROJECT_DIR}/compat_reports/lib${lib_name}/${PREVIOUS_SDK_VERSION_NUM}_to_Y/compat_report.html" ${CI_PROJECT_DIR}/compat_report_${lib_name}.html
done

# Show list of reports, will be saved as artifacts
ls -la ${CI_PROJECT_DIR}/*.html
