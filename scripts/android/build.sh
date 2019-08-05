#!/bin/bash
mv android-9 android-9_source && mkdir -p android-9/platforms/android-28 && cp -r android-9_source/* android-9/platforms/android-28/ && rm -rf android-9_source
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="$WORKSPACE/android-ndk-r20/build/cmake/android.toolchain.cmake"  -DANDROID_ABI=arm64-v8a -DEDGE_SDK_ENABLE_TESTING=NO -DEDGE_SDK_BUILD_EXAMPLES=ON
make
