#!/bin/bash -ex

BUILD_DIR="build-clang-tidy"

rm -rf "${BUILD_DIR}"
mkdir "${BUILD_DIR}"

cmake -S . -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_CXX_COMPILER=clang++-17 \
    -DCMAKE_C_COMPILER=clang-17 \
    -DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations" \
    -DOLP_SDK_ENABLE_TESTING=OFF \
    -DOLP_SDK_BUILD_EXAMPLES=OFF

cmake --build "${BUILD_DIR}" -- -j"$(nproc)"

run-clang-tidy-17.py -p "${BUILD_DIR}" \
    -config-file="${PWD}/.clang-tidy" \
    "${PWD}/olp-cpp-sdk-core/.*" \
    "${PWD}/olp-cpp-sdk-authentication/.*" \
    "${PWD}/olp-cpp-sdk-dataservice-read/.*" \
    "${PWD}/olp-cpp-sdk-dataservice-write/.*"