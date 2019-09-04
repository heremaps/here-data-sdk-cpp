#!/bin/bash
CPP_TEST_SOURCE_AUTHENTICATION=build/olp-cpp-sdk-authentication/test
CPP_TEST_SOURCE_CORE=build/olp-cpp-sdk-core/tests
CPP_TEST_SOURCE_DARASERVICE_READ=build/olp-cpp-sdk-dataservice-read/test
CPP_TEST_SOURCE_DARASERVICE_WRITE=build/olp-cpp-sdk-dataservice-write/test
RC=0
echo ">>> Authentication Test ... >>>"
$CPP_TEST_SOURCE_AUTHENTICATION/unit/olp-authentication-test --gtest_output="xml:report.xml" --gtest_filter="AuthenticationOfflineTest.*" || RC=1
echo ">>> Core - Cache Test ... >>>"
$CPP_TEST_SOURCE_CORE/cache/cache-test --gtest_output="xml:report1.xml" || RC=2
echo ">>> Core - Client Test ... >>>"
$CPP_TEST_SOURCE_CORE/client/client-test --gtest_output="xml:report2.xml" || RC=3
echo ">>> Core - Geo Test ... >>>"
$CPP_TEST_SOURCE_CORE/geo/geo-test --gtest_output="xml:report3.xml" || RC=4
echo ">>> Core - Network Test ... >>>"
$CPP_TEST_SOURCE_CORE/network/olp-core-network-test --gtest_output="xml:report4.xml" --gtest_filter="*Offline*:*offline*" || RC=5
echo ">>> Core - Thread Test ... >>>"
$CPP_TEST_SOURCE_CORE/thread/thread-test --gtest_output="xml:report5.xml" || RC=6
echo ">>> Dataservice read Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_READ/unit/olp-dataservice-read-test --gtest_output="xml:report6.xml" --gtest_filter=-"TestOnline/*" || RC=7
echo ">>> Dataservice write Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_WRITE/olp-dataservice-write-test --gtest_output="xml:report7.xml" --gtest_filter=-"*Online*":"TestCacheMock*" || RC=8
bash <(curl -s https://codecov.io/bash)
exit $RC
