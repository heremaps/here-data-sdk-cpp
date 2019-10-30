#!/bin/bash -e

echo ">>> Core - SOURCE DATASERVICE WRITE Test ... >>>"
source $FV_HOME/olp-cpp-sdk-dataservice-write-test.variables
$REPO_HOME/build/olp-cpp-sdk-dataservice-write/tests/olp-cpp-sdk-dataservice-write-tests \
    --gtest_output="xml:$REPO_HOME/reports/olp-dataservice-write-test-report.xml"