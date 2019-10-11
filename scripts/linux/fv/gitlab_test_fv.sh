#!/bin/bash -e
# Running every test one by one

export REPO_HOME=$PWD
export FV_HOME=${REPO_HOME}/scripts/linux/fv
rm -rf reports && mkdir -p reports              # folder for reports storage

source ${FV_HOME}/olp-common.variables          # variables common for all tests
if [ -z $production_service_secret ] || [ -z $production_service_id ] ; then
    echo "FATAL FAILURE: production_service_id and production_service_secret are not defined. Have you defined env.variables on GitLab?"
fi

TEST_FAILURE=0
EXPECTED_REPORT_COUNT=6                         # expected that we generate 6 reports

# Run unit tests
${FV_HOME}/gitlab-olp-cpp-sdk-authentication-test.sh 2>> errors.txt || TEST_FAILURE=1
${FV_HOME}/gitlab-olp-cpp-sdk-core-test.sh 2>> errors.txt || TEST_FAILURE=1
${FV_HOME}/gitlab-olp-dataservice-read-test.sh 2>> errors.txt || TEST_FAILURE=1
${FV_HOME}/gitlab-olp-dataservice-write-test.sh 2>> errors.txt || TEST_FAILURE=1

# Run integration tests
${FV_HOME}/gitlab-olp-cpp-sdk-integration-test.sh 2>> errors.txt || TEST_FAILURE=1

# Run functional tests
${FV_HOME}/gitlab-olp-cpp-sdk-functional-test.sh 2>> errors.txt || TEST_FAILURE=1

# Lines below are added for pretty data sum-up and finalize results of this script is case of FAILURE
if [[ ${TEST_FAILURE} == 1 ]]; then
    export REPORT_COUNT=$(ls ${REPO_HOME}/reports | wc -l)
    if [[ ${REPORT_COUNT} -ne ${EXPECTED_REPORT_COUNT} || ${REPORT_COUNT} == 0 ]]; then
        echo "CRASH ERROR. One of test groups contains crash. Report was not generated for that group ! "
    fi
else
    echo "OK. Full list of test reports was generated. "
fi


for failreport in `ls ${REPO_HOME}/reports/*-report.xml`
do
    if [[ $(grep -q "<failure" ${failreport}) ]]; then
        echo "${failreport} contains errors : "
        cat ${failreport}
        echo "Full log contains errors : "
        cat errors.txt
    fi
done

echo "Summary ejected from reports is : "
for report in `ls ${REPO_HOME}/reports/*-report.xml`
do
    echo -e "$(basename ${report}): \t $(cat ${report} | sed -n 2p | sed -e "s/timestamp=.*//" | sed -e "s/\<testsuites//" )"
done

echo "Artifacts download URL: https://main.gitlab.in.here.com/olp/edge/olp-sdk/olp-sdk-cpp/-/jobs/$CI_JOB_ID/artifacts/download"

exit ${TEST_FAILURE}
