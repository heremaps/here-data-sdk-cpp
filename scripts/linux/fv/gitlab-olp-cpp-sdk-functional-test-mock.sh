#!/bin/bash -ex
#
# Copyright (C) 2020 HERE Europe B.V.
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

# This script is for running functional tests against Mock Server
# https://www.mock-server.com/

# For core dump backtrace
ulimit -c unlimited

echo ">>> Starting Mock Server... >>>"
pushd tests/utils/mock-server
npm install
node server.js & export SERVER_PID=$!
popd

# Node can start server in ~1 second, but not faster.
# Add waiter for server to be started. No other way to solve that.
# When curl returns code 1 , means server still down. Curl returns 0 when server is up.
RC=1
while [[ ${RC} -ne 0 ]];
do
        set +e
        curl -s http://localhost:1080
        RC=$?
        sleep 0.3
        set -e
done

echo ">>> Installing mock server SSL certificate into OS... >>>"
curl https://raw.githubusercontent.com/mock-server/mockserver/master/mockserver-core/src/main/resources/org/mockserver/socket/CertificateAuthorityCertificate.pem --output mock-server-cert.pem
mv mock-server-cert.pem /usr/share/ca-certificates/
echo "mock-server-cert.pem" >> /etc/ca-certificates.conf
update-ca-certificates

echo ">>> Starting Functional Test against Mock Server... >>>"
$REPO_HOME/build/tests/functional/olp-cpp-sdk-functional-tests \
    --gtest_output="xml:$REPO_HOME/reports/olp-functional-test-mock-report.xml" \
    --gtest_filter="VersionedLayerClientTest.GetPartitions":"VersionedLayerClientTest.GetAggregatedData":"CatalogClientTest.*":"VersionedLayerClientPrefetchTest.Prefetch":"VersionedLayerClientProtectTest.*":"VersionedLayerClientGetDataTest.*"
result=$?
echo "Functional test with Mock finished with status: ${result}"

# Terminate the mock server
kill -TERM $SERVER_PID
wait

exit ${result}
