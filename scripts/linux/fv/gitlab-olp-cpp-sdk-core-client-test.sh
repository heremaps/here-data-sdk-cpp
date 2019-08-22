#!/bin/bash -e

echo ">>> Core - Client Test ... >>>"
source $FV_HOME/olp-cpp-sdk-core-client-test.variables
$REPO_HOME/build/olp-cpp-sdk-core/tests/client/client-test --gtest_output="xml:$REPO_HOME/reports/olp-core-client-test-report.xml"