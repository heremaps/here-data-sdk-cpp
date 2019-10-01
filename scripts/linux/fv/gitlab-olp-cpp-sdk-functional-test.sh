#!/bin/bash -e

#Start local server
node $REPO_HOME/tests/utils/mock_server/server.js & export SERVER_PID=$!

# Node can start server in 1 second, but not faster.
# Add waiter for server to be started. No other way to solve that.
# Curl returns code 1 - means server still down. Curl returns 0 when server is up
RC=1
while [ $RC -ne 0 ];
do
        set +e
        curl -s http://localhost:3000
        RC=$?
        sleep 0.15
        set -e
done

echo ">>> Functional Test ... >>>"
source $FV_HOME/olp-cpp-sdk-functional-test.variables
$REPO_HOME/build/tests/functional/olp-cpp-sdk-functional-tests \
    --gtest_output="xml:$REPO_HOME/reports/olp-functional-test-report.xml"

#Kill local server
kill -15 $SERVER_PID
wait $SERVER_PID # Waiter for server process to be exited correctly