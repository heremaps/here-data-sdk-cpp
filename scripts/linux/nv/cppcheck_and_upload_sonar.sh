#!/bin/bash -e
#
# Copyright (C) 2019-2021 HERE Europe B.V.
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

# This is script for CPPcheck scan and SonarQube scan(including valgrind report from previous job)

export REPO_HOME=$PWD
export FV_HOME=${REPO_HOME}/scripts/linux/fv
export NV_HOME=${REPO_HOME}/scripts/linux/nv

# cpp check tool
cppcheck_folder="${REPO_HOME}/reports/cppcheck"
mkdir -p "$cppcheck_folder"

echo "#############################"
echo "Cppcheck run : "
echo "#############################"

cppcheck \
    -v -j 4 --enable=all --xml --xml-version=2 --enable=all --inconclusive  \
    -DHAVE_SIGNAL_H \
    -DIGNORE_SIGPIPE \
    -DOLP_SDK_NETWORK_HAS_CURL \
    -DOLP_SDK_NETWORK_HAS_OPENSSL \
    -DOLP_SDK_NETWORK_HAS_PIPE=1 \
    -DOLP_SDK_NETWORK_HAS_UNISTD_H=1 \
    -DOLP_SDK_PLATFORM_NAME=\"Linux\" \
    -DTHREAD_LIBRARY \
    -DTHREAD_LIBRARY_DYNAMIC \
    -I olp-cpp-sdk-core/include \
    -I olp-cpp-sdk-authentication/include \
    -I olp-cpp-sdk-dataservice-read/include \
    -I olp-cpp-sdk-dataservice-write/include \
    olp-cpp-sdk-core/src \
    olp-cpp-sdk-authentication/src \
    olp-cpp-sdk-dataservice-read/src \
    olp-cpp-sdk-dataservice-write/src \
    2>cppcheck.xml
cat cppcheck.xml
mv cppcheck.xml "$cppcheck_folder/cppcheck.xml"

# Set needed properties for Valgrind
echo "sonar.host.url=${SONAR_HOST}" >> ${NV_HOME}/sonar-project.propertiesValgrind
echo "sonar.login=${SONAR_TOKEN}" >> ${NV_HOME}/sonar-project.propertiesValgrind
cp ${NV_HOME}/sonar-project.propertiesValgrind ${REPO_HOME}/sonar-project.properties

echo "#############################"
echo "Sonar scanner run : "
echo "#############################"
$SONAR_SCANNER_ROOT/sonar-scanner -X -Dsonar.verbose=true
