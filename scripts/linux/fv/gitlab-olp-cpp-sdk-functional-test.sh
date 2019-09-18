#!/bin/bash -e

echo ">>> Functional Test ... >>>"
source $FV_HOME/olp-cpp-sdk-functional-test.variables
$REPO_HOME/build/tests/functional/olp-cpp-sdk-functional-tests \
    --gtest_output="xml:$REPO_HOME/reports/olp-functional-test-report.xml"
