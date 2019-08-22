#!/bin/bash -e

echo ">>> Core - Geo Test ... >>>"
source $FV_HOME/olp-cpp-sdk-core-geo-test.variables
$REPO_HOME/build/olp-cpp-sdk-core/tests/geo/geo-test --gtest_output="xml:$REPO_HOME/reports/olp-core-geo-test-report.xml"