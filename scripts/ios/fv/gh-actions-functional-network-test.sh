#!/bin/bash -ex
#
# Copyright (C) 2021 HERE Europe B.V.
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

###
### Script starts Mock server, install certificate, add it to trusted and
### run FV functional test against it inside iOS Simulator (more info in OLPEDGE-1773)
### NOTE: Simulator fails during application launch, so step is disabled until OLPEDGE-2543 is DONE.
###
### # TODO in OLPEDGE-2543 :
##  1. Find out how to save xml report after Simulator test done.
##  2. Fix `xcrun simctl launch` cmd below: errors from app like -999 etc.
##  3. Functional test should be able to use this variable : IP_ADDR for connecting to Mock Server. It's possible solution for error -999 when connecting to Server.
##  4. Need to find out how to parse `xcrun simctl spawn booted log stream` and catch finish of test-run. Otherwise it goes to infinite loop.
##
## export IP_ADDR=$(ipconfig getifaddr en0)
## curl -s http://${IP_ADDR}:1080
##
##
## For linux fv network tests please refer to : ./scripts/linux/fv/gitlab-olp-cpp-sdk-functional-network-test.sh


# For core dump backtrace
ulimit -c unlimited

# Set workspace location
if [[ ${CI_PROJECT_DIR} == "" ]]; then
    export CI_PROJECT_DIR=$(pwd)
fi

if [[ ${GITHUB_RUN_ID} == "" ]]; then
    # Local run
    export GITHUB_RUN_ID=$(date '+%s')
    pushd tests/utils/mock-server
      echo ">>> Installing mock server SSL certificate into OS... >>>"
      # Import and Make trusted
      security import mock-server-cert.pem || true
      security add-trusted-cert -d -r trustRoot -k /Library/Keychains/System.keychain mock-server-cert.pem || true
      # Validate cert: if trusted - succeeded , if not trusted - fails
      security verify-cert -c mock-server-cert.pem
      echo ">>> Starting Mock Server... >>>"
      npm install
      node server.js & export SERVER_PID=$!
    popd
else
    # CI run
    pushd tests/utils/mock-server
      echo ">>> Installing mock server SSL certificate into OS... >>>"
      # Import and Make trusted
      sudo security import mock-server-cert.pem
      sudo security authorizationdb write com.apple.trust-settings.admin allow; sudo security add-trusted-cert -d -r trustRoot -k /Library/Keychains/System.keychain mock-server-cert.pem ; sudo security authorizationdb remove com.apple.trust-settings.admin || true
      # Validate cert: if trusted - succeeded , if not trusted - fails
      sudo security verify-cert -c mock-server-cert.pem
      echo ">>> Starting Mock Server... >>>"
      npm install
      node server.js & export SERVER_PID=$!
    popd
fi


# Node can start server in 1 second, but not faster.
# Add waiter for server to be started. No other way to solve that.
# Curl returns code 1 - means server still down. Curl returns 0 when server is up
RC=1
while [[ ${RC} -ne 0 ]];
do
        set +e
        curl -s http://localhost:1080
        RC=$?
        sleep 0.2
        set -e
done

# Functional test should be able to use this variable : IP_ADDR for connecting to Mock Server.
# Looks like localhost is not reachable for Simulator.
export IP_ADDR=$(ipconfig getifaddr en0)
curl -s http://${IP_ADDR}:1080

echo ">>> Start network tests ... >>>"
# List available devices, runtimes on MacOS node
xcrun simctl list
xcrun simctl list devices
xcrun simctl list runtimes

export CurrentDeviceUDID=$(xcrun simctl list devices | grep "iPhone 11 (" | grep -v "unavailable" | grep -v "com.apple.CoreSimulator.SimDeviceType" | cut -d'(' -f2 | cut -d')' -f1 | head -1)

# Create new Simulator device

xcrun simctl list devices | grep "iPhone 11"
xcrun simctl boot "iPhone 11" || true
xcrun simctl list devices | grep "iPhone 11"
xcrun simctl create ${GITHUB_RUN_ID}_iphone11 "com.apple.CoreSimulator.SimDeviceType.iPhone-11"
xcrun simctl list devices | grep "iPhone 11"
echo "Simulator created"

#/Applications/Xcode_11.2.1.app/Contents/Developer/Applications/Simulator.app/Contents/MacOS/Simulator -CurrentDeviceUDID ${CurrentDeviceUDID} & export SIMULATOR_PID=$! || /Applications/Xcode.app/Contents/Developer/Applications/Simulator.app/Contents/MacOS/Simulator -CurrentDeviceUDID ${CurrentDeviceUDID} & export SIMULATOR_PID=$!
#echo "Simulator started device"
#
#xcrun simctl logverbose enable
#xcrun simctl install ${CurrentDeviceUDID} ./build/tests/functional/network/ios/olp-cpp-sdk-functional-network-tests-tester/Debug-iphonesimulator/olp-ios-olp-cpp-sdk-functional-network-tests-lib-tester.app
#echo "App installed"
#xcrun simctl launch ${CurrentDeviceUDID} com.here.olp.olp-ios-olp-cpp-sdk-functional-network-tests-lib-tester
# Need to find out how to parse `xcrun simctl spawn booted log stream` and catch finish of test-run. Otherwise it goes to infinite loop.
# xcrun simctl spawn booted log stream | grep "olp-ios-olp-cpp-sdk-functional-network-tests-lib-tester:" | tee app.log
#result=$?
#echo "App launched"
#xcrun simctl shutdown ${CurrentDeviceUDID}
#echo "Simulator shutdown done"

#echo ">>> Finished network tests >>>"

# Terminate the mock server
#kill -TERM $SERVER_PID || true
#kill -TERM $SIMULATOR_PID || true

#wait

#exit ${result}
