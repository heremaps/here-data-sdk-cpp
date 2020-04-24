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


# Variables
export REPO_HOME=$PWD
export FV_HOME=${REPO_HOME}/scripts/linux/fv
rm -rf reports && mkdir -p reports              # folder for reports storage

source ${FV_HOME}/olp-common.variables          # variables common for all tests
if [ -z $production_service_secret ] || [ -z $production_service_id ] ; then
    echo "FATAL FAILURE: production_service_id and production_service_secret are not defined. Have you defined env.variables on GitLab?"
fi

TEST_FAILURE=0
RETRY_COUNT=0
EXPECTED_REPORT_COUNT=6                         # expected that we generate 6 reports

#for core dump backtrace
ulimit -c unlimited

# Running every test one by one
# Functional tests must run first,
# as we have retry mechanism which causes problems with other tests results if they run before functional.

# Run functional tests
# Add retry to functional/online tests. Some online tests are flaky due to third party reason
# Test failure must return code 1 only. Other return code are not handled in retry.
# Allowing tests failures (exit codes not 0) for online tests is done by set command below. Process is controlled by retry mechanism below.
# Stop after 2nd retry.
source ${FV_HOME}/olp-cpp-sdk-functional-test.variables
set +e
for RETRY_COUNT in 1 2
do
	echo "This is (${RETRY_COUNT}) time run ..."

	# Run functional tests
	${FV_HOME}/gitlab-olp-cpp-sdk-functional-test.sh

	if [[ $? -eq 1 ]]; then
	    # Functional test failed with exit code 1 means some tests failed
		TEST_FAILURE=1
		sleep 10
	else
	    # Return to success
		TEST_FAILURE=0
        break
	fi
done
set -e
# End of retry part. This part can be removed any time later or after all online tests are stable.

# Run integration tests
source ${FV_HOME}/olp-cpp-sdk-integration-test.variables
${FV_HOME}/gitlab-olp-cpp-sdk-integration-test.sh || TEST_FAILURE=1

# Run unit tests
source ${FV_HOME}/olp-cpp-sdk-authentication-test.variables
${FV_HOME}/gitlab-olp-cpp-sdk-authentication-test.sh || TEST_FAILURE=1

source ${FV_HOME}/olp-cpp-sdk-core-test.variables
${FV_HOME}/gitlab-olp-cpp-sdk-core-test.sh || TEST_FAILURE=1

source ${FV_HOME}/olp-cpp-sdk-dataservice-read-test.variables
${FV_HOME}/gitlab-olp-cpp-sdk-dataservice-read-test.sh || TEST_FAILURE=1

source ${FV_HOME}/olp-cpp-sdk-dataservice-write-test.variables
${FV_HOME}/gitlab-olp-cpp-sdk-dataservice-write-test.sh || TEST_FAILURE=1


# Lines below are added for pretty data sum-up and finalize results of this script is case of FAILURE
set +x  # to avoid dirty output at the end on logs
if [[ ${TEST_FAILURE} == 1 ]]; then
    export REPORT_COUNT=$(ls ${REPO_HOME}/reports | wc -l)
    if [[ ${REPORT_COUNT} -ne ${EXPECTED_REPORT_COUNT} || ${REPORT_COUNT} == 0 ]]; then
        echo "##################################################################################################"
        echo "CRASH ERROR. One of test groups contains crash. Report was not generated for that group ! "
        echo "##################################################################################################"
    fi
else
    echo "OK. Full list of test reports was generated. "
fi


for failreport in $(ls ${REPO_HOME}/reports/*.xml)
do
    echo "Parsing ${failreport} ..."
    if $(grep -q "<failure" "${failreport}" ) ; then
        echo "${failreport} contains errors : "
        echo "Printing report ##################################################################################"
        cat ${failreport}
        echo "End of report ####################################################################################"
    fi
done

echo "Summary ejected from reports is : "
for report in `ls ${REPO_HOME}/reports/*-report.xml`
do
    echo -e "$(basename ${report}): \t $(cat ${report} | sed -n 2p | sed -e "s/timestamp=.*//" | sed -e "s/\<testsuites//" )"
done

echo "Artifacts download URL: ${CI_PROJECT_URL}-/jobs/${CI_JOB_ID}/artifacts/download"

exit ${TEST_FAILURE}
