#!/bin/bash -xe
#for core dump backtrace
ulimit -c unlimited

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DOLP_SDK_BUILD_EXAMPLES=ON \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    ..

make -j$(nproc)
ccache -s
