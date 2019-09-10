#!/bin/bash -e
CPP_TEST_SOURCE_AUTHENTICATION=build/olp-cpp-sdk-authentication/test
CPP_TEST_SOURCE_CORE=build/olp-cpp-sdk-core/tests
CPP_TEST_SOURCE_DARASERVICE_READ=build/olp-cpp-sdk-dataservice-read/test
CPP_TEST_SOURCE_DARASERVICE_WRITE=build/olp-cpp-sdk-dataservice-write/test
echo ">>> Authentication Test ... >>>"
$CPP_TEST_SOURCE_AUTHENTICATION/unit/olp-authentication-test --gtest_output="xml:report.xml" --gtest_filter="AuthenticationOfflineTest.*"
echo ">>> Core - Cache Test ... >>>"
$CPP_TEST_SOURCE_CORE/cache/cache-test --gtest_output="xml:report1.xml"
echo ">>> Core - Client Test ... >>>"
$CPP_TEST_SOURCE_CORE/client/client-test --gtest_output="xml:report2.xml"
echo ">>> Core - Geo Test ... >>>"
$CPP_TEST_SOURCE_CORE/geo/geo-test --gtest_output="xml:report3.xml"
echo ">>> Core - Thread Test ... >>>"
$CPP_TEST_SOURCE_CORE/thread/thread-test --gtest_output="xml:report5.xml"
echo ">>> Dataservice read Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_READ/unit/olp-dataservice-read-test --gtest_output="xml:report6.xml" --gtest_filter=-"TestOnline/*"
echo ">>> Dataservice write Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_WRITE/olp-dataservice-write-test --gtest_output="xml:report7.xml" --gtest_filter=-"*Online*"
bash <(curl -s https://codecov.io/bash)

