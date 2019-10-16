#!/bin/bash -e

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
