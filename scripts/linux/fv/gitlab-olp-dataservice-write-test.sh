#!/bin/bash -e

echo ">>> Core - SOURCE DATASERVICE WRITE Test ... >>>"
source $FV_HOME/olp-dataservice-write-test.variables
$REPO_HOME/build/olp-cpp-sdk-dataservice-write/test/olp-dataservice-write-test --gtest_output="xml:$REPO_HOME/reports/olp-dataservice-write-test-report.xml"