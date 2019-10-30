#!/bin/bash -e

export REPO_HOME=$PWD
export FV_HOME=${REPO_HOME}/scripts/linux/fv
export NV_HOME=${REPO_HOME}/scripts/linux/nv

# cpp check tool
cppcheck_folder="${REPO_HOME}/reports/cppcheck"
mkdir -p "$cppcheck_folder"

cppcheck --xml --xml-version=2 --enable=all --inconclusive -I olp-cpp-sdk-authentication/include/olp/authentication/ olp-cpp-sdk-authentication/src 2> cppcheck.xml
cp cppcheck.xml "$cppcheck_folder/olp-cpp-sdk-authentication-cppcheck.xml"

cppcheck --xml --xml-version=2 --enable=all --inconclusive -I olp-cpp-sdk-dataservice-read/include/olp/dataservice/read/ \
-I olp-cpp-sdk-dataservice-read/include/olp/dataservice/read/model/ olp-cpp-sdk-dataservice-read/src 2>cppcheck.xml
cp cppcheck.xml "$cppcheck_folder/olp-cpp-sdk-dataservice-read-cppcheck.xml"

cppcheck --xml --xml-version=2 --enable=all --inconclusive -I olp-cpp-sdk-dataservice-write/include/olp/dataservice/write/ \
-I olp-cpp-sdk-dataservice-write/include/olp/dataservice/write/model/  \
-I olp-cpp-sdk-dataservice-write/include/olp/dataservice/write/generated/model/ olp-cpp-sdk-dataservice-write/src 2>cppcheck.xml
cp cppcheck.xml "$cppcheck_folder/olp-cpp-sdk-dataservice-write-cppcheck.xml"

cppcheck --xml --xml-version=2 --enable=all --inconclusive -I olp-cpp-sdk-core/include/olp/core/client/ \
-I olp-cpp-sdk-core/include/olp/core/context/ \
-I olp-cpp-sdk-core/include/olp/core/cache/ \
-I olp-cpp-sdk-core/include/olp/core/geo/ \
-I olp-cpp-sdk-core/include/olp/core/http/ \
-I olp-cpp-sdk-core/include/olp/core/logging/ \
-I olp-cpp-sdk-core/include/olp/core/math/ \
-I olp-cpp-sdk-core/include/olp/core/generated/parser/ \
-I olp-cpp-sdk-core/include/olp/core/generated/serializer/ \
-I olp-cpp-sdk-core/include/olp/core/porting/ \
-I olp-cpp-sdk-core/include/olp/core/thread/  \
-I olp-cpp-sdk-core/include/olp/core/utils/ olp-cpp-sdk-core/src 2>cppcheck.xml
cp cppcheck.xml "$cppcheck_folder/olp-cpp-sdk-core-cppcheck.xml"

# Set needed properties for Valgrind
echo "sonar.host.url=${SONAR_HOST}" >> ${NV_HOME}/sonar-project.propertiesValgrind
cp ${NV_HOME}/sonar-project.propertiesValgrind ${REPO_HOME}/sonar-project.properties

SONAR_SCANNER_ROOT="/home/bldadmin/sonarqube-linux/sonar-scanner-3.2.0.1227-linux/bin"

echo "#############################"
echo "Sonar scanner run : "
echo "#############################"
$SONAR_SCANNER_ROOT/sonar-scanner -X
