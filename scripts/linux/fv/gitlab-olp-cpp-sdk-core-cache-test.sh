#!/bin/bash -e

echo ">>> Core - Cache Test ... >>>"
source $FV_HOME/olp-cpp-sdk-core-cache-test.variables
$REPO_HOME/build/olp-cpp-sdk-core/tests/cache/cache-test --gtest_output="xml:$REPO_HOME/reports/olp-core-cache-test-report.xml"