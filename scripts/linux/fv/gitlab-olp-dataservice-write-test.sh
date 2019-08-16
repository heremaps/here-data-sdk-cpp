#!/bin/bash -e

source $FV_HOME/olp-dataservice-write-test.variables
echo ">>> Core - SOURCE DATASERVICE WRITE Test ... >>>"
env
$REPO_HOME/build/olp-cpp-sdk-dataservice-write/test/olp-dataservice-write-test --gtest_output="xml:$REPO_HOME/reports/olp-dataservice-write-test-report.xml"