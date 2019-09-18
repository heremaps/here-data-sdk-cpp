#!/bin/bash -e

echo ">>> Integration Test ... >>>"
source $FV_HOME/olp-cpp-sdk-integration-test.variables
$REPO_HOME/build/tests/integration/olp-cpp-sdk-integration-tests \
    --gtest_output="xml:$REPO_HOME/reports/olp-integration-test-report.xml"
