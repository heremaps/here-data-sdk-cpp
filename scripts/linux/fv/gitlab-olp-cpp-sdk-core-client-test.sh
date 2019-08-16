#!/bin/bash -e

source $FV_HOME/olp-cpp-sdk-core-client-test.variables
echo ">>> Core - Client Test ... >>>"
$REPO_HOME/build/olp-cpp-sdk-core/tests/client/client-test --gtest_output="xml:$REPO_HOME/reports/olp-core-client-test-report.xml"
