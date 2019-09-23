#!/bin/bash -e
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -OLP_SDK_BUILD_EXAMPLES=ON -DOLP_SDK_BUILD_DOC=ON -DBUILD_SHARED_LIBS=ON ..
make -j8
make docs
cd ..
