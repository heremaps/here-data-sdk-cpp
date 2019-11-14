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


# For core dump backtrace
ulimit -c unlimited

CPP_TEST_SOURCE_AUTHENTICATION=build/olp-cpp-sdk-authentication/tests
CPP_TEST_SOURCE_CORE=build/olp-cpp-sdk-core/tests
CPP_TEST_SOURCE_DARASERVICE_READ=build/olp-cpp-sdk-dataservice-read/tests
CPP_TEST_SOURCE_DARASERVICE_WRITE=build/olp-cpp-sdk-dataservice-write/tests
CPP_TEST_SOURCE_INTEGRATION=build/tests/integration
echo ">>> Authentication Test ... >>>"
$CPP_TEST_SOURCE_AUTHENTICATION/olp-cpp-sdk-authentication-tests \
    --gtest_output="xml:olp-cpp-sdk-authentication-tests-report.xml" \
    --gtest_filter="AuthenticationOfflineTest.*"
echo ">>> Core Test ... >>>"
$CPP_TEST_SOURCE_CORE/olp-cpp-sdk-core-tests \
    --gtest_output="xml:olp-cpp-sdk-core-tests-report.xml"
echo ">>> Dataservice read Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_READ/olp-cpp-sdk-dataservice-read-tests \
    --gtest_output="xml:olp-cpp-sdk-dataservice-read-tests-report.xml"
echo ">>> Dataservice write Test ... >>>"
$CPP_TEST_SOURCE_DARASERVICE_WRITE/olp-cpp-sdk-dataservice-write-tests \
    --gtest_output="xml:olp-cpp-sdk-dataservice-write-tests-report.xml"
echo ">>> Integration Test ... >>>"
$CPP_TEST_SOURCE_INTEGRATION/olp-cpp-sdk-integration-tests \
    --gtest_output="xml:olp-cpp-sdk-integration-tests-report.xml"

bash <(curl -s https://codecov.io/bash)
