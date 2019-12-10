#!/bin/bash -ex
#
# Copyright (C) 2019 HERE Europe B.V.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
# License-Filename: LICENSE


# Start local server
node $REPO_HOME/tests/utils/mock_server/server.js & export SERVER_PID=$!

# Node can start server in 1 second, but not faster.
# Add waiter for server to be started. No other way to solve that.
# Curl returns code 1 - means server still down. Curl returns 0 when server is up
RC=1
while [[ ${RC} -ne 0 ]];
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
    --gtest_output="xml:$REPO_HOME/reports/olp-functional-test-report.xml" \
    --gtest_filter="-ArcGisAuthenticationTest.SignInArcGis":"FacebookAuthenticationTest.SignInFacebook"
export result=$?

# Kill local server
kill -15 ${SERVER_PID}
# Waiter for server process to be exited correctly
wait ${SERVER_PID}
echo "Functional test finished with status: ${result}"
exit ${result}
