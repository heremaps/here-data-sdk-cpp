#!/bin/bash -xe
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -OLP_SDK_BUILD_EXAMPLES=ON \
    -DBUILD_SHARED_LIBS=ON \
    ..
make -j
cd ..
