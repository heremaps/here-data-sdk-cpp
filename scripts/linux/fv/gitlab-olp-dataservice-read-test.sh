#!/bin/bash -e

echo ">>> Core - SOURCE DATASERVICE READ Test ... >>>"
source $FV_HOME/olp-dataservice-read-test.variables
$REPO_HOME/build/olp-cpp-sdk-dataservice-read/test/unit/olp-dataservice-read-test --gtest_output="xml:$REPO_HOME/reports/olp-dataservice-read-test-report.xml"
