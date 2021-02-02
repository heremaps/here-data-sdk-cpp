#!/bin/bash -ex
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

echo ">>> Functional Test ... >>>"
$REPO_HOME/build/tests/functional/olp-cpp-sdk-functional-tests \
    --gtest_output="xml:$REPO_HOME/reports/olp-functional-test-report.xml" \
    --gtest_filter="-ArcGisAuthenticationTest.SignInArcGis":"FacebookAuthenticationTest.SignInFacebook":"VersionedLayerClientTest.GetPartitions":"VersionedLayerClientTest.GetAggregatedData":"CatalogClientTest.*":"VersionedLayerClientPrefetchTest.*":"VersionedLayerClientProtectTest.*":"VersionedLayerClientGetDataTest.*"
    #The test VersionedLayerClientTest.GetPartitions uses mock server and it will be started in separate script. (OLPEDGE-732)
result=$?
echo "Functional test finished with status: ${result}"

exit ${result}
