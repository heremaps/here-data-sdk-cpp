#!/bin/bash -e
mkdir -p build && cd build
cmake ../ -GXcode \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/iOS.cmake \
    -DPLATFORM=iphoneos \
    -DOLP_SDK_ENABLE_TESTING=NO \
    -DSIMULATOR=YES \
    -DOLP_SDK_BUILD_EXAMPLES=ON

xcodebuild
cd ..
