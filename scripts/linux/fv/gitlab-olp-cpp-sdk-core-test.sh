#!/bin/bash -e

echo ">>> Core Test ... >>>"
source $FV_HOME/olp-cpp-sdk-core-test.variables
$REPO_HOME/build/olp-cpp-sdk-core/tests/olp-cpp-sdk-core-tests \
    --gtest_output="xml:$REPO_HOME/reports/olp-core-test-report.xml" \
    --gtest_filter=-"DefaultCacheTest.BadPath"
