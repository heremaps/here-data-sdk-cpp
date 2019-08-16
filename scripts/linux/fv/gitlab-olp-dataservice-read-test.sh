#!/bin/bash -e

source $FV_HOME/olp-dataservice-read-test.variables
echo ">>> Core - SOURCE DATASERVICE READ Test ... >>>"
$REPO_HOME/build/olp-cpp-sdk-dataservice-read/test/unit/olp-dataservice-read-test --gtest_output="xml:$REPO_HOME/reports/olp-dataservice-read-test-report.xml" --gtest_filter="-TestMock/CatalogClientMockTest.GetPartitions403CacheClear/0"