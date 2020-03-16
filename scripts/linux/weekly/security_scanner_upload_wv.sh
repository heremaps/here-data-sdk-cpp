#!/bin/bash -ex
#
# Copyright (C) 2020 HERE Europe B.V.
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
#
###### Script for packaging and uploading to Security Scanner for security test. Mandatory for Security review.
#
# Required variables:
# SECURITY_API_PWD, SECURITY_API_USER, SECURITY_SCANNER, SECURITY_API_URL .
# note that below hardcoded is number 454830 : CPP SDK app ID in Security Scanner .
#
# SDK Usage:
# SecurityScannerUpload.sh build/binaries.tar.gz name.surname@example.com # any mail can be specified
#
###### Usage from official Security Scanner API documentation :
# SecurityScannerUpload.sh <filname> <email> <application-id> <label>
# Examples:
# SecurityScannerUpload.sh /tmp/app.war name.surname@example.com 443849 1.0.0-RC1
#
set +e
LATEST_TAG=$(git describe --abbrev=0 --tags) || LATEST_TAG=$(git describe --abbrev=0 --tags --always)
LATEST_HASH=$(git rev-parse --short=7 HEAD)
set -e
echo "##############################"
echo "LATEST_TAG is ${LATEST_TAG}"
echo "##############################"
echo "LATEST_HASH is ${LATEST_HASH}"
echo "##############################"

# Check packing all files for further scan, skip tests and git files.
tar -tvf build/binaries.tar.gz

echo "Packed file: $1. Uploading it..."

curl -f -v -k -H"X-spc-${SECURITY_SCANNER}-username:${SECURITY_API_USER}" -H"X-spc-${SECURITY_SCANNER}-password:${SECURITY_API_PWD}" \
-H"X-spc-${SECURITY_SCANNER}-email:$2" -F"file=@$1" https://${SECURITY_API_URL}/${SECURITY_SCANNER}-ws/analysis/start/454830/version/edge-sdk-cpp-${LATEST_HASH}

echo "File $1 was uploaded to Security Scanner via API"
