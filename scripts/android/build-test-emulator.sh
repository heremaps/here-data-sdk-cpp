#!/bin/bash -ex
#
# Copyright (C) 2020-2025 HERE Europe B.V.
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

${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --list
${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --install "ndk;22.1.7171670" --sdk_root=${ANDROID_HOME} >/dev/null
${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --install "platforms;android-21"
${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --install "platform-tools"
${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --install "ndk-bundle"
${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --list

#
# API / ABI / NDK are hardcoded below
#
# Variables
export ANDROID_ABI="x86_64"
export ANDROID_API=21
export ANDROID_NDK_ROOT="/usr/local/lib/android/sdk/ndk/22.1.7171670"
export ANDROID_NDK="/usr/local/lib/android/sdk/ndk/22.1.7171670"
export ANDROID_NDK_HOME="/usr/local/lib/android/sdk/ndk/22.1.7171670"
ls -la android-ndk-r22b-linux || true
ls -la /usr/local/lib/android/sdk/ndk/22.1.7171670 || true
ls -la /usr/local/lib/android/sdk/ndk/22.1.7171670/android-ndk-r22b-linux || true
export NDK_ROOT=$ANDROID_NDK_ROOT/ndk-bundle                        # This var is not exist on Azure MacOS image, step can be skipped on other CI
echo "NDK_ROOT is ${NDK_ROOT} , ANDROID_HOME is ${ANDROID_HOME} "   # as we already set this var inside docker image.
ls -la $ANDROID_HOME
export PATH=$PATH:$ANDROID_HOME/tools/bin/:$ANDROID_HOME/emulator:$ANDROID_HOME/platform-tools:$ANDROID_HOME/cmdline-tools/latest/bin/

# Start compilation
mkdir -p build && cd build

echo ""
echo ""
echo "*************** $VARIANT Build SDK for C++ ********** Start ***************"
CMAKE_COMMAND="cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TYPE=${BUILD_TYPE} -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
-DANDROID_PLATFORM=android-$ANDROID_API -DANDROID_STL=c++_static -DANDROID_ABI=$ANDROID_ABI"
BUILD_COMMAND="cmake --build . -- -j4"

echo ""
echo " ---- Calling $CMAKE_COMMAND"
${CMAKE_COMMAND}

# Run CMake. Warnings and errors are saved to build/CMakeFiles/CMakeOutput.log and
# build/CMakeFiles/CMakeError.log.
# -- We link Edge SDK as shared libraries in order to use shadowing for unit tests.
# -- We build the examples.
echo ""
echo " ---- Calling ${BUILD_COMMAND}"
${BUILD_COMMAND}
cd -

${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --list

# Add emulator if not already added. Needed for docker.
echo "y" | ${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager "emulator" "platforms;android-$ANDROID_API"

# Install AVD files
echo "y" | ${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --install "system-images;android-$ANDROID_API;google_apis;$ANDROID_ABI"

# Create emulator
echo "yes" | ${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/avdmanager create avd -n android_emulator -k "system-images;android-$ANDROID_API;google_apis;$ANDROID_ABI" --force
echo "AVD created"
emulator -list-avds

cd $ANDROID_HOME/emulator
echo "Starting emulator in background"
nohup emulator -avd android_emulator -no-snapshot -noaudio \
-no-boot-anim -gpu off -no-accel -no-window -camera-back none -camera-front none -selinux permissive \
-qemu -m 2048 &

# Below are special commands for wait until emulator actually loads and boot completed
adb wait-for-device
A=$(adb shell getprop init.svc.adbd | tr -d '\r' )
while [ "$A" != "running" ]; do  sleep 3; A=$(adb shell getprop init.svc.adbd | tr -d '\r' );  done

# Showing list of devices connected to host
adb devices

# At this moment we assume that Android Virtual Device is started in emulation, ready for our commands.
# Running some trivial command like pressing Menu button.
adb shell input keyevent 82

# example of network usage check:
# to find network usage for the app 'com.example.myapp', run the following commands:
# adb shell dumpsys package com.example.myapp | grep userId
# adb shell dumpsys connectivity

# Showing list of devices connected to host
adb devices

### In this place, we plan to run .apk as test application inside emulator.
echo "Emulator started"
