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


# This script executes NightlyVerification tests by running FullVerification tests with Valgrind
# Some tests are skipped due to flakiness, find then in EXCEPTION variable below.

export REPO_HOME=$PWD
export FV_HOME=${REPO_HOME}/scripts/linux/fv
export NV_HOME=${REPO_HOME}/scripts/linux/nv
mkdir -p reports/valgrind # for valgrind reports storage
export misc_folder_path="reports/valgrind"
TEST_TARGET_NAMES=(
    "olp-cpp-sdk-authentication-test"
    "olp-cpp-sdk-core-test"
    "olp-cpp-sdk-dataservice-read-test"
    "olp-cpp-sdk-dataservice-write-test")
source ${FV_HOME}/olp-common.variables          # variables common for all tests
if [ -z $production_service_secret ] || [ -z $production_service_id ] ; then
    echo "FATAL FAILURE: production_service_id and production_service_secret are not defined. Have you defined env.variables on GitLab?"
    exit 1
fi
ulimit -c unlimited # for core dump backtrace

cd build

# Start local server
node ${REPO_HOME}/tests/utils/mock_server/server.js & export SERVER_PID=$!

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
echo ">>> Local Server started for further functional test ... >>>"

### Running two auto-test groups
for test_group_name in integration functional
do
{
    if [[ ${test_group_name} == "functional" ]] ; then
        EXCEPTION="--gtest_filter=-ArcGisAuthenticationTest.SignInArcGis:FacebookAuthenticationTest.SignInFacebook:DataserviceReadVersionedLayerClientTest.Prefetch:DataserviceReadVersionedLayerClientTest.PrefetchWithCancellableFuture"
    elif [[ ${test_group_name} == "integration" ]] ; then
        EXCEPTION="--gtest_filter=-DataserviceReadVersionedLayerClientTest.PrefetchTilesBusy:DataserviceReadVersionedLayerClientTest.PrefetchTilesWithCancellableFuture:DataserviceReadVersionedLayerClientTest.PrefetchTilesWithCache:DataserviceReadVersionedLayerClientTest.Prefetch"
    else
        EXCEPTION=""
        FLAKY_CHECK=""
    fi

    source ${FV_HOME}/olp-cpp-sdk-${test_group_name}-test.variables
    test_command="$REPO_HOME/build/tests/${test_group_name}/olp-cpp-sdk-${test_group_name}-tests --gtest_output=xml:${REPO_HOME}/reports/olp-cpp-sdk-${test_group_name}-test-report.xml ${EXCEPTION} ${FLAKY_CHECK}"
    valgrind_command="valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all --xml=yes"
    if [[ ! -z "$misc_folder_path" ]]; then
        valgrind_result_file_path="${REPO_HOME}/${misc_folder_path}/${test_group_name}_memcheck.xml"
        valgrind_command="${valgrind_command} --xml-file=${valgrind_result_file_path}"
    fi
    test_command="export GLIBCXX_FORCE_NEW=; ${valgrind_command} ${test_command}"

    echo "-----> Calling \"${test_command}\" for ${test_group_name} : "
    eval "${test_command}"
    result=$?
    echo "-----> Finished ${test_group_name} - Result=${result}"

    # Add retry to functional/online tests. Some online tests are flaky due to third party reason
    # Test failure must return code 1 only. Other return code are not handled in retry.
    RETRY_COUNT=0
    while true
    do
        if [[ ${test_group_name} == "functional" ]]; then
            # Stop after 3 retry
            if [[ ${RETRY_COUNT} -eq 3 ]]; then
                echo "Reach limit (${RETRY_COUNT}) of retries ..."
                break
            fi
            RETRY_COUNT=$((RETRY_COUNT+1))
            echo "This is ${RETRY_COUNT} time retry ..."

            # Run functional tests
            echo "-----> Calling \"${test_command}\" for ${test_group_name} : "
            eval "${test_command}"
            result=$?
            echo "-----> Finished ${test_group_name} - Result=${result}"
            if [[ ${result} -eq 1 ]]; then
                TEST_FAILURE=1
                continue
            else
                # Return to success
                TEST_FAILURE=0
                break
            fi
        fi
        break
    done
    # End of retry part. This part can be removed anytime.
}
done

### Running all unit groups
for test_name in "${TEST_TARGET_NAMES[@]}"
do
{
    if [[ ${test_name} == "olp-cpp-sdk-core-test" ]] ; then
        EXCEPTION="--gtest_filter="-DefaultCacheTest.BadPath""
    else
        EXCEPTION=""
    fi
    FLAKY_CHECK=""
    source ${FV_HOME}/${test_name}.variables
    test_command="$REPO_HOME/build/${test_name%?????}/tests/${test_name}s \
    --gtest_output="xml:${REPO_HOME}/reports/${test_name}-report.xml" $EXCEPTION $FLAKY_CHECK "

    valgrind_command="valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all --xml=yes"
    if [[ ! -z "$misc_folder_path" ]]; then
        valgrind_result_file_path="${REPO_HOME}/${misc_folder_path}/${test_name}_memcheck.xml"
        valgrind_command="$valgrind_command --xml-file=$valgrind_result_file_path"
    fi
    test_command="export GLIBCXX_FORCE_NEW=; $valgrind_command $test_command"

    echo "-----> Calling $test_command for $test_name : "
    eval "${test_command}"
    result=$?
    echo "-----> Finished $test_name - Result=$result"
}
done


cd ..

ls -la reports/
ls -la reports/valgrind

for failreport in $(ls ${REPO_HOME}/reports/*.xml)
do
    echo "Parsing ${failreport} ..."
    if $(grep -q "<failure" "${failreport}" ) ; then
        echo "${failreport} contains failed tests : "
        cat ${failreport}
    fi
done

echo "Summary ejected from reports is : "
for report in `ls ${REPO_HOME}/reports/*-report.xml`
do
    echo -e "$(basename ${report}): \t $(cat ${report} | sed -n 2p | sed -e "s/timestamp=.*//" | sed -e "s/\<testsuites//" )"
done

echo "Artifacts download URL: ${CI_PROJECT_URL}-/jobs/${CI_JOB_ID}/artifacts/download"

if [[ ${result} -ne 0 ]]; then
    exit ${result}
fi
