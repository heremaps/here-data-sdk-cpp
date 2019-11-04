/*
 * Copyright (C) 2019 HERE Europe B.V.
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

#include "olp/core/utils/WarningWorkarounds.h"

#ifdef NETWORK_HAS_CURL
#include "curl/NetworkCurl.h"
#elif NETWORK_HAS_ANDROID
#include "android/NetworkAndroid.h"
#elif NETWORK_HAS_IOS
#include "ios/OLPNetworkIOS.h"
#elif NETWORK_HAS_WINHTTP
#include "winhttp/NetworkWinHttp.h"
#endif

namespace olp {
namespace http {

CORE_API std::shared_ptr<Network> CreateDefaultNetwork(
    size_t max_requests_count) {
  CORE_UNUSED(max_requests_count);
#ifdef NETWORK_HAS_CURL
  return std::make_shared<NetworkCurl>(max_requests_count);
#elif NETWORK_HAS_ANDROID
  return std::make_shared<NetworkAndroid>(max_requests_count);
#elif NETWORK_HAS_IOS
  return std::make_shared<OLPNetworkIOS>(max_requests_count);
#elif NETWORK_HAS_WINHTTP
  return std::make_shared<NetworkWinHttp>(max_requests_count);
#else
  static_assert(false, "No default network implementation provided");
#endif
}

}  // namespace http
}  // namespace olp
