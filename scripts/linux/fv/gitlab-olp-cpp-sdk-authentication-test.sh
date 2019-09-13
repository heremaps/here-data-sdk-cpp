#!/bin/bash -e

echo ">>> Core - Authentication Test ... >>>"
source $FV_HOME/olp-cpp-sdk-core-authentication-test.variables
$REPO_HOME/build/olp-cpp-sdk-authentication/tests/olp-cpp-sdk-authentication-tests \
    --gtest_output="xml:$REPO_HOME/reports/olp-authentication-test-report.xml" \
    --gtest_filter="-ArcGisAuthenticationOnlineTest.SignInArcGis"