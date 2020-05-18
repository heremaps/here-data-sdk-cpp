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


#mkdir -p build && cd build
#cmake .. -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
#	-DANDROID_PLATFORM=android-25 \
#	-DANDROID_ABI=arm64-v8a \
#	-DOLP_SDK_ENABLE_TESTING=NO \
#	-DOLP_SDK_BUILD_EXAMPLES=ON

#cmake --build . # this is alternative option for build
#sudo make install -j$(nproc)
#cd -

ls -la $ANDROID_HOME
export PATH=$PATH:$ANDROID_HOME/tools/bin/
sdkmanager --list

# Install AVD files
echo "y" | $ANDROID_HOME/tools/bin/sdkmanager --install 'system-images;android-27;google_apis;x86'

# Create emulator
echo "no" | $ANDROID_HOME/tools/bin/avdmanager create avd -n xamarin_android_emulator -k 'system-images;android-27;google_apis;x86' --force

$ANDROID_HOME/emulator/emulator -list-avds

echo "Starting emulator"

# Start emulator in background
nohup $ANDROID_HOME/emulator/emulator -avd xamarin_android_emulator -no-snapshot > /dev/null 2>&1 &
$ANDROID_HOME/platform-tools/adb wait-for-device shell 'while [[ -z $(getprop sys.boot_completed | tr -d '\r') ]]; do sleep 1; done; input keyevent 82'

$ANDROID_HOME/platform-tools/adb devices

echo "Emulator started"

#sdkmanager "platform-tools" "platforms;android-25" "emulator"
#sdkmanager "system-images;android-25;google_apis;arm64-v8a"
#echo no | avdmanager create avd -n android-25 -k "system-images;android-25;google_apis;arm64-v8a"
#emulator -avd android-25 -no-snapshot -noaudio -no-boot-anim -gpu off -no-accel -no-window -camera-back none -camera-front none -selinux permissive -qemu -m 2048 &
#./scripts/android/android-wait-for-emulator.sh
#adb shell input keyevent 82 &