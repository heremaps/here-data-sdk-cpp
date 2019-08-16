#!/bin/bash -e

source $FV_HOME/olp-cpp-sdk-core-geo-test.variables
echo ">>> Core - Geo Test ... >>>"
$REPO_HOME/build/olp-cpp-sdk-core/tests/geo/geo-test --gtest_output="xml:$REPO_HOME/reports/olp-core-geo-test-report.xml"
