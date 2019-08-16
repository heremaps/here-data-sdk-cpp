#!/bin/bash -e
#Running every test one by one

export REPO_HOME=$PWD
export FV_HOME=$REPO_HOME/scripts/linux/fv
rm -rf reports && mkdir -p reports #for reports storage

source $FV_HOME/olp-common.variables #variables common for all tests
if [ -z $production_service_secret ] || [ -z $production_service_id ] ; then
    echo "FATAL FAILURE: production_service_id and production_service_secret are not defined on Gitlab or during your local build"
fi

$FV_HOME/gitlab-olp-cpp-sdk-authentication-test.sh
$FV_HOME/gitlab-olp-cpp-sdk-core-cache-test.sh
$FV_HOME/gitlab-olp-cpp-sdk-core-client-test.sh
$FV_HOME/gitlab-olp-cpp-sdk-core-geo-test.sh
$FV_HOME/gitlab-olp-cpp-sdk-core-network-test.sh
$FV_HOME/gitlab-olp-dataservice-read-test.sh
$FV_HOME/gitlab-olp-dataservice-write-test.sh

ls -la $REPO_HOME/reports/

