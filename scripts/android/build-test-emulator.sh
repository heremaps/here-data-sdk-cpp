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
# API and ABI are hardcoded there
#
# Android Only Variables
export ANDROID_ABI="x86"
export ANDROID_API=28:
export NDK_ROOT=$ANDROID_HOME/ndk-bundle    # This var is not exist on Azure MacOS image, step can be skipped on GitLab
echo "NDK_ROOT is ${NDK_ROOT} "             # as we already set this var inside docker image.
ls -la $ANDROID_HOME
export PATH=$PATH:$ANDROID_HOME/tools/bin/

mkdir -p build && cd build

echo ""
echo ""
echo "*************** $VARIANT Build SDK for C++ ********** Start ***************"
CMAKE_COMMAND="cmake .. -DCMAKE_TOOLCHAIN_FILE=$NDK_ROOT/build/cmake/android.toolchain.cmake \
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

sdkmanager --list

# Install AVD files
echo "y" | sdkmanager --install "system-images;android-$ANDROID_API;google_apis;$ANDROID_ABI"

# Create emulator
echo "no" | avdmanager create avd -n android_emulator -k "system-images;android-$ANDROID_API;google_apis;$ANDROID_ABI" --force
echo "AVD created"
$ANDROID_HOME/emulator/emulator -list-avds

echo "Starting emulator in background"
# Start emulator
nohup $ANDROID_HOME/emulator/emulator -avd android_emulator -no-snapshot -noaudio \
-no-boot-anim -gpu off -no-accel -no-window -camera-back none -camera-front none -selinux permissive \
-qemu -m 2048 > /dev/null 2>&1 &

# Below are special commands for wait until emulator actually loads and boot completed
$ANDROID_HOME/platform-tools/adb wait-for-device
A=$($ANDROID_HOME/platform-tools/adb shell getprop sys.boot_completed | tr -d r)
while [ "$A" != "1" ]; do
        sleep 5
        A=$($ANDROID_HOME/platform-tools/adb shell getprop sys.boot_completed | tr -d r)
done

# At this moment we assume that Android Virtual Device is started in emulation, ready for our commands.
# Running some trivial command like pressing Menu button.
$ANDROID_HOME/platform-tools/adb shell input keyevent 82

# Showing list of devices connected to host
$ANDROID_HOME/platform-tools/adb devices

### In this place, we plan to run .apk as test application inside emulator.
echo "Emulator started"
