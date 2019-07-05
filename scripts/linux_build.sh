#!/bin/bash -xe
cd mnt/test
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -EDGE_SDK_BUILD_EXAMPLES=ON ..
make -j8
cd ..
CPP_TEST_SOURCE_AUTHENTICATION=build/olp-cpp-sdk-authentication/test
CPP_TEST_SOURCE_CORE=build/olp-cpp-sdk-core/tests
CPP_O="$(find build/* | grep cpp.o)"
tar -czf artifacts.tar.gz --exclude='*.git' \
    "build/lib" \
    `find build -type f -executable` \
echo ">>> Authentication Test ... >>>"
$CPP_TEST_SOURCE_AUTHENTICATION/unit/olp-authentication-test --gtest_output="xml:report.xml" --gtest_filter="AuthenticationOfflineTest.*"
echo ">>> Core - Cache Test ... >>>"
$CPP_TEST_SOURCE_CORE/cache/cache-test --gtest_output="xml:report1.xml"
echo ">>> Core - Geo Test ... >>>"
$CPP_TEST_SOURCE_CORE/geo/geo-test --gtest_output="xml:report2.xml"
echo ">>> Core - Network Test ... >>>"
$CPP_TEST_SOURCE_CORE/network/olp-core-network-test --gtest_output="xml:report3.xml" --gtest_filter="*Offline*:*offline*"
