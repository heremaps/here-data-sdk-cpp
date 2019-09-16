#!/bin/bash -e
CPP_TEST_SOURCE_AUTHENTICATION=build/olp-cpp-sdk-authentication/tests
CPP_TEST_SOURCE_CORE=build/olp-cpp-sdk-core/tests
CPP_TEST_SOURCE_DARASERVICE_READ=build/olp-cpp-sdk-dataservice-read/tests
CPP_TEST_SOURCE_DARASERVICE_WRITE=build/olp-cpp-sdk-dataservice-write/tests
CPP_TEST_SOURCE_FUNCTIONAL=build/tests/functional
echo ">>> Authentication Test ... >>>"
$CPP_TEST_SOURCE_AUTHENTICATION/olp-cpp-sdk-authentication-tests \
    --gtest_output="xml:olp-cpp-sdk-authentication-tests-report.xml" \
    --gtest_filter="AuthenticationOfflineTest.*"
echo ">>> Core Test ... >>>"
$CPP_TEST_SOURCE_CORE/olp-cpp-sdk-core-tests \
    --gtest_output="xml:olp-cpp-sdk-core-tests-report.xml"
echo ">>> Dataservice read Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_READ/olp-cpp-sdk-dataservice-read-tests \
    --gtest_output="xml:olp-cpp-sdk-dataservice-read-tests-report.xml" \
    --gtest_filter=-"TestOnline/*"
echo ">>> Dataservice write Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_WRITE/olp-cpp-sdk-dataservice-write-tests \
    --gtest_output="xml:olp-cpp-sdk-dataservice-write-tests-report.xml" \
    --gtest_filter=-"*Online*":"TestCacheMock*"
echo ">>> Functional Test ... >>>"
node ./testutils/mock_server/server.js & SERVER_PID=$!
$CPP_TEST_SOURCE_FUNCTIONAL/olp-cpp-sdk-functional-tests \
    --gtest_output="xml:olp-cpp-sdk-functional-tests-report.xml"
kill -15 $SERVER_PID

bash <(curl -s https://codecov.io/bash)
