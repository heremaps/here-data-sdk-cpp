#!/bin/bash -e
CPP_TEST_SOURCE_AUTHENTICATION=build/olp-cpp-sdk-authentication/tests
CPP_TEST_SOURCE_CORE=build/olp-cpp-sdk-core/tests
CPP_TEST_SOURCE_DARASERVICE_READ=build/olp-cpp-sdk-dataservice-read/tests
CPP_TEST_SOURCE_DARASERVICE_WRITE=build/olp-cpp-sdk-dataservice-write/tests
CPP_TEST_SOURCE_INTEGRATION=build/tests/integration
echo ">>> Authentication Test ... >>>"
$CPP_TEST_SOURCE_AUTHENTICATION/olp-cpp-sdk-authentication-tests \
    --gtest_output="xml:olp-cpp-sdk-authentication-tests-report.xml" \
    --gtest_filter="AuthenticationOfflineTest.*"
echo ">>> Core Test ... >>>"
$CPP_TEST_SOURCE_CORE/olp-cpp-sdk-core-tests \
    --gtest_output="xml:olp-cpp-sdk-core-tests-report.xml"
echo ">>> Dataservice read Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_READ/olp-cpp-sdk-dataservice-read-tests \
    --gtest_output="xml:olp-cpp-sdk-dataservice-read-tests-report.xml"
echo ">>> Dataservice write Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_WRITE/olp-cpp-sdk-dataservice-write-tests \
    --gtest_output="xml:olp-cpp-sdk-dataservice-write-tests-report.xml" \
    --gtest_filter=-"*Online*":"TestCacheMock*"
echo ">>> Integration Test ... >>>"
$CPP_TEST_SOURCE_INTEGRATION/olp-cpp-sdk-integration-tests \
    --gtest_output="xml:olp-cpp-sdk-integration-tests-report.xml" \
    --gtest_filter="CatalogClient*"

bash <(curl -s https://codecov.io/bash)
