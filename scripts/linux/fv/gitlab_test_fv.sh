#!/bin/bash -e
#Running every test one by one

export REPO_HOME=$PWD
export FV_HOME=$REPO_HOME/scripts/linux/fv
rm -rf reports && mkdir -p reports #for reports storage

source $FV_HOME/olp-common.variables #variables common for all tests
if [ -z $production_service_secret ] || [ -z $production_service_id ] ; then
    echo "FATAL FAILURE: production_service_id and production_service_secret are not defined"
fi

TEST_FAILURE=0

$FV_HOME/gitlab-olp-cpp-sdk-authentication-test.sh || TEST_FAILURE=1
$FV_HOME/gitlab-olp-cpp-sdk-core-cache-test.sh || TEST_FAILURE=1
$FV_HOME/gitlab-olp-cpp-sdk-core-client-test.sh || TEST_FAILURE=1
$FV_HOME/gitlab-olp-cpp-sdk-core-geo-test.sh || TEST_FAILURE=1
$FV_HOME/gitlab-olp-cpp-sdk-core-network-test.sh || TEST_FAILURE=1
$FV_HOME/gitlab-olp-dataservice-read-test.sh || TEST_FAILURE=1
$FV_HOME/gitlab-olp-dataservice-write-test.sh || TEST_FAILURE=1

if [[ $TEST_FAILURE==1 ]]; then
    echo "Some tests failed."
fi

exit $TEST_FAILURE