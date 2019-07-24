#!/bin/bash
mv android-5.0 android-5.0_source && mkdir -p android-5.0/platforms/android-21 && cp -r android-5.0_source/* android-5.0/platforms/android-21/ && rm -rf android-5.0_source
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="$WORKSPACE/android-ndk-r19c/build/cmake/android.toolchain.cmake" -DANDROID_ABI=arm64-v8a -DEDGE_SDK_ENABLE_TESTING=NO
make
