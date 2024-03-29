#!/bin/bash -ex
#
# Copyright (C) 2021 HERE Europe B.V.
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

# Script run olp-cpp-sdk-performance-tests with metrics collecting

# For core dump backtrace
ulimit -c unlimited

# Start local server
node tests/utils/olp_server/server.js & export SERVER_PID=$!

# Node can start server in 1 second, but not faster.
# Add waiter for server to be started. No other way to solve that.
# Curl returns code 1 - means server still down. Curl returns 0 when server is up
RC=1
while [[ ${RC} -ne 0 ]];
do
        set +e
        curl -s http://localhost:3000
        RC=$?
        sleep 0.2
        set -e
done
echo ">>> Local Server started for further performance test ... >>>"

# Measure Disk I/O and CPU/RAM
# Run test with metrics collector
python3 $CI_PROJECT_DIR/scripts/linux/weekly/run_performance_test_metrics.py

# Terminate the OLP server
kill -TERM $SERVER_PID

wait
