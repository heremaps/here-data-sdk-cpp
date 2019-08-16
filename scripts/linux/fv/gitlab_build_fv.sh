#!/bin/bash -xe
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -EDGE_SDK_BUILD_EXAMPLES=ON -DEDGE_SDK_BUILD_DOC=ON ..
make -j8
make docs
cd ..
