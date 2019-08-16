#!/bin/bash -e

source $FV_HOME/olp-cpp-sdk-core-cache-test.variables
echo ">>> Core - Cache Test ... >>>"
$REPO_HOME/build/olp-cpp-sdk-core/tests/cache/cache-test --gtest_output="xml:$REPO_HOME/reports/olp-core-cache-test-report.xml"
