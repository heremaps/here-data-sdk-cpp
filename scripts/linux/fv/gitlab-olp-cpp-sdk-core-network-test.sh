#!/bin/bash -e

source $FV_HOME/olp-cpp-sdk-core-network-test.variables
echo ">>> Core - Network Test ... >>>"
$REPO_HOME/build/olp-cpp-sdk-core/tests/network/olp-core-network-test --gtest_output="xml:$REPO_HOME/reports/olp-core-network-test-report.xml"