#!/bin/bash -e
CPP_TEST_SOURCE_AUTHENTICATION=build/olp-cpp-sdk-authentication/test
CPP_TEST_SOURCE_CORE=build/olp-cpp-sdk-core/tests
CPP_TEST_SOURCE_DARASERVICE_READ=build/olp-cpp-sdk-dataservice-read/test
CPP_TEST_SOURCE_DARASERVICE_WRITE=build/olp-cpp-sdk-dataservice-write/test
echo ">>> Authentication Test ... >>>"
$CPP_TEST_SOURCE_AUTHENTICATION/unit/olp-authentication-test --gtest_output="xml:olp-authentication-test.xml" --gtest_filter="AuthenticationOfflineTest.*"
echo ">>> Core - Cache Test ... >>>"
$CPP_TEST_SOURCE_CORE/cache/cache-test --gtest_output="xml:cache-test.xml"
echo ">>> Core - Client Test ... >>>"
$CPP_TEST_SOURCE_CORE/client/client-test --gtest_output="xml:client-test.xml"
echo ">>> Core - Geo Test ... >>>"
$CPP_TEST_SOURCE_CORE/geo/geo-test --gtest_output="xml:geo-test.xml"
echo ">>> Core - Thread Test ... >>>"
$CPP_TEST_SOURCE_CORE/thread/thread-test --gtest_output="xml:thread-test.xml"
echo ">>> Core - Network Test ... >>>"
$CPP_TEST_SOURCE_CORE/http/network-test --gtest_output="xml:network-test.xml"
echo ">>> Dataservice read Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_READ/unit/olp-dataservice-read-test --gtest_output="xml:olp-dataservice-read-test.xml" --gtest_filter=-"TestOnline/*"
echo ">>> Dataservice write Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_WRITE/olp-dataservice-write-test --gtest_output="xml:olp-dataservice-write-test.xml" --gtest_filter=-"*Online*":"TestCacheMock*"
bash <(curl -s https://codecov.io/bash)

