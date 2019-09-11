#!/bin/bash -e
CPP_TEST_SOURCE_AUTHENTICATION=build/olp-cpp-sdk-authentication/tests
CPP_TEST_SOURCE_CORE=build/olp-cpp-sdk-core/tests
CPP_TEST_SOURCE_DARASERVICE_READ=build/olp-cpp-sdk-dataservice-read/test
CPP_TEST_SOURCE_DARASERVICE_WRITE=build/olp-cpp-sdk-dataservice-write/test
echo ">>> Authentication Test ... >>>"
$CPP_TEST_SOURCE_AUTHENTICATION/olp-cpp-sdk-authentication-tests \
    --gtest_output="xml:report.xml" \
    --gtest_filter="AuthenticationOfflineTest.*"
echo ">>> Core Test ... >>>"
$CPP_TEST_SOURCE_CORE/olp-cpp-sdk-core-tests --gtest_output="xml:report1.xml"
echo ">>> Dataservice read Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_READ/unit/olp-dataservice-read-test --gtest_output="xml:report2.xml" --gtest_filter=-"TestOnline/*"
echo ">>> Dataservice write Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_WRITE/olp-dataservice-write-test --gtest_output="xml:report3.xml" --gtest_filter=-"*Online*":"TestCacheMock*"
bash <(curl -s https://codecov.io/bash)
