#!/bin/bash -e
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

ulimit -c unlimited # for core dump backtrace

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
        sleep 0.15
        set -e
done
echo ">>> Local Server started for further performance test ... >>>"

export cache_location="cache"

heaptrack ./build/tests/performance/olp-cpp-sdk-performance-tests --gtest_filter="*short_test_null_cache"
heaptrack ./build/tests/performance/olp-cpp-sdk-performance-tests --gtest_filter="*short_test_memory_cache"
heaptrack ./build/tests/performance/olp-cpp-sdk-performance-tests --gtest_filter="*short_test_disk_cache"

du -h $cache_location

# TODO:
#  1. print the total allocations done
#  2. generate nice looking graphs using heaptrack_print and flamegraph.pl
#  3. use watch command on cache directory, like: watch "du | cut -d'.' -f1 >> cache.csv" and plot a disk size graph
#  4. track the disk IO made by SDK
#  5. track the CPU load

#Kill local server
kill -15 $SERVER_PID
wait $SERVER_PID # Waiter for server process to be exited correctly