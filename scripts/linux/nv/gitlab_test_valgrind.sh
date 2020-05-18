#!/bin/bash -ex
#
# Copyright (C) 2019-2020 HERE Europe B.V.
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

# This script executes NightlyVerification tests by running FullVerification tests with Valgrind.
# Functional tests are skipped at all.
# Some tests might be skipped due to flakiness, find then in EXCEPTION variable below.

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

###
### Running test groups
###

for test_group_name in integration
do
{
    EXCEPTION=""
    FLAKY_CHECK=""
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

    }
done

###
### Running all unittest groups
###

for test_name in "${TEST_TARGET_NAMES[@]}"
do
{
    EXCEPTION=""
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

set +x  # to avoid dirty output at the end on logs
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

echo "Artifacts download URL: ${CI_PROJECT_URL}/-/jobs/${CI_JOB_ID}/artifacts/download"

if [[ ${result} -ne 0 ]]; then
    exit ${result}
fi
