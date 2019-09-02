#!/bin/bash -xe
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -EDGE_SDK_BUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=ON ..
make -j8
cd ..
