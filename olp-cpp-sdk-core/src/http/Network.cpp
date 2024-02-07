/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

#include "olp/core/http/Network.h"

#include "http/DefaultNetwork.h"
#include "olp/core/utils/WarningWorkarounds.h"

#ifdef OLP_SDK_NETWORK_OFFLINE
#include "offline/NetworkOffline.h"
#elif OLP_SDK_NETWORK_HAS_CURL
#include "curl/NetworkCurl.h"
#elif OLP_SDK_NETWORK_HAS_ANDROID
#include "android/NetworkAndroid.h"
#elif OLP_SDK_NETWORK_HAS_IOS
#include "ios/OLPNetworkIOS.h"
#elif OLP_SDK_NETWORK_HAS_WINHTTP
#include "winhttp/NetworkWinHttp.h"
#endif

namespace olp {
namespace http {

namespace {
std::shared_ptr<Network> CreateDefaultNetworkImpl(
    NetworkInitializationSettings settings) {
  OLP_SDK_CORE_UNUSED(settings);
#ifdef OLP_SDK_NETWORK_OFFLINE
  return std::make_shared<NetworkOffline>();
#elif OLP_SDK_NETWORK_HAS_CURL
  return std::make_shared<NetworkCurl>(settings);
#elif OLP_SDK_NETWORK_HAS_ANDROID
  return std::make_shared<NetworkAndroid>(settings.max_requests_count);
#elif OLP_SDK_NETWORK_HAS_IOS
  return std::make_shared<OLPNetworkIOS>(settings.max_requests_count);
#elif OLP_SDK_NETWORK_HAS_WINHTTP
  return std::make_shared<NetworkWinHttp>(settings.max_requests_count);
#else
  static_assert(false, "No default network implementation provided");
#endif
}
}  // namespace

void Network::SetDefaultHeaders(Headers /*headers*/) {}

void Network::SetCurrentBucket(uint8_t /*bucket_id*/) {}

Network::Statistics Network::GetStatistics(uint8_t /*bucket_id*/) {
  return Network::Statistics{};
}

std::shared_ptr<Network> CreateDefaultNetwork(size_t max_requests_count) {
  NetworkInitializationSettings settings;
  settings.max_requests_count = max_requests_count;
  return CreateDefaultNetwork(std::move(settings));
}

std::shared_ptr<Network> CreateDefaultNetwork(
    NetworkInitializationSettings settings) {
  auto network = CreateDefaultNetworkImpl(std::move(settings));
  if (network) {
    return std::make_shared<DefaultNetwork>(network);
  }
  return nullptr;
}

}  // namespace http
}  // namespace olp
