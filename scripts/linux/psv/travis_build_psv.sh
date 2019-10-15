#!/bin/bash -xe
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DOLP_SDK_BUILD_EXAMPLES=ON \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    ..

make -j8
cd ..
ccache -s
