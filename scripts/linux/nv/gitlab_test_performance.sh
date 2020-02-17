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

# For core dump backtrace
ulimit -c unlimited

# Start MockServer
nohup node tools/mock-server/server.js > mock-server-out.log 2>mock-server-error.log &

# Wait until it's up.
# Curl returns code 1 - means server still down. Curl returns 0 when server is up
RC=1
while [[ ${RC} -ne 0 ]];
do
        set +e
        curl -s http://localhost:1080
        RC=$?
        sleep 0.2
        set -e
done
echo ">>> MockServer started ... >>>"

export cache_location="cache"

echo ">>> Start performance tests ... >>>"
heaptrack ./build/tests/performance/olp-cpp-sdk-performance-tests --gtest_filter="*ReadNPartitionsFromVersionedLayer/short_test_null_cache" 2>> errors.txt || TEST_FAILURE=1
mv heaptrack.olp-cpp-sdk-performance-tests.*.gz heaptrack.ReadNPartitionsFromVersionedLayer.short_test_null_cache.gz

heaptrack ./build/tests/performance/olp-cpp-sdk-performance-tests --gtest_filter="*ReadNPartitionsFromVersionedLayer/short_test_memory_cache" 2>> errors.txt || TEST_FAILURE=1
mv heaptrack.olp-cpp-sdk-performance-tests.*.gz heaptrack.ReadNPartitionsFromVersionedLayer.short_test_memory_cache.gz

heaptrack ./build/tests/performance/olp-cpp-sdk-performance-tests --gtest_filter="*ReadNPartitionsFromVersionedLayer/short_test_disk_cache" 2>> errors.txt || TEST_FAILURE=1
mv heaptrack.olp-cpp-sdk-performance-tests.*.gz heaptrack.ReadNPartitionsFromVersionedLayer.short_test_disk_cache.gz

heaptrack ./build/tests/performance/olp-cpp-sdk-performance-tests --gtest_filter="*PrefetchPartitionsFromVersionedLayer/short_test_null_cache" 2>> errors.txt || TEST_FAILURE=1
mv heaptrack.olp-cpp-sdk-performance-tests.*.gz heaptrack.PrefetchPartitionsFromVersionedLayer.short_test_null_cache.gz

heaptrack ./build/tests/performance/olp-cpp-sdk-performance-tests --gtest_filter="*PrefetchPartitionsFromVersionedLayer/short_test_memory_cache" 2>> errors.txt || TEST_FAILURE=1
mv heaptrack.olp-cpp-sdk-performance-tests.*.gz heaptrack.PrefetchPartitionsFromVersionedLayer.short_test_memory_cache.gz

heaptrack ./build/tests/performance/olp-cpp-sdk-performance-tests --gtest_filter="*PrefetchPartitionsFromVersionedLayer/short_test_disk_cache" 2>> errors.txt || TEST_FAILURE=1
mv heaptrack.olp-cpp-sdk-performance-tests.*.gz heaptrack.PrefetchPartitionsFromVersionedLayer.short_test_disk_cache.gz
echo ">>> Finished performance tests . >>>"

if [[ ${TEST_FAILURE} == 1 ]]; then
        echo "Printing error.txt ###########################################"
        cat errors.txt
        echo "End of error.txt #############################################"
        echo "CRASH ERROR. One of test groups contains crash. Report was not generated for that group ! "
else
    echo "OK. Full list of tests passed. "
fi

mkdir reports/heaptrack
mkdir heaptrack

# Third party dependency needed for pretty graph generation below
git clone --depth=1 https://github.com/brendangregg/FlameGraph

# test names
t1=short_test_disk_cache
t2=short_test_memory_cache
t3=short_test_null_cache

for archive_name in PrefetchPartitionsFromVersionedLayer.$t1 PrefetchPartitionsFromVersionedLayer.$t2 PrefetchPartitionsFromVersionedLayer.$t3 ReadNPartitionsFromVersionedLayer.$t1 ReadNPartitionsFromVersionedLayer.$t2 ReadNPartitionsFromVersionedLayer.$t3
do
    heaptrack_print --print-leaks \
      --print-flamegraph heaptrack/flamegraph_${archive_name}.data \
      --file heaptrack.${archive_name}.gz > reports/heaptrack/report_${archive_name}.txt
    # Pretty graph generation
    ./FlameGraph/flamegraph.pl --title="Flame Graph: ${archive_name}" heaptrack/flamegraph_${archive_name}.data > reports/heaptrack/flamegraph_${archive_name}.svg
    cat reports/heaptrack/flamegraph_${archive_name}.svg >> heaptrack_report.html
done
cp heaptrack_report.html reports
ls -la heaptrack
ls -la

du ${cache_location}
du ${cache_location}/*
ls -la ${cache_location}

# TODO:
#  1. print the total allocations done
#  2. use watch command on cache directory, like: watch "du | cut -d'.' -f1 >> cache.csv" and plot a disk size graph
#  3. track the disk IO made by SDK
#  4. track the CPU load

# Termiante the mock server
kill -TERM $SERVER_PID

wait
