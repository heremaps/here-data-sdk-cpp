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

#include "olp/core/porting/make_unique.h"
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

CORE_API std::unique_ptr<Network> CreateDefaultNetwork(size_t max_requests_count) {
#ifdef NETWORK_HAS_CURL
  CORE_UNUSED(max_requests_count);
  return std::make_unique<NetworkCurl>();
#elif NETWORK_HAS_ANDROID
  CORE_UNUSED(max_requests_count);
  return std::make_unique<NetworkAndroid>();
#elif NETWORK_HAS_IOS
  return std::make_unique<OLPNetworkIOS>(max_requests_count);
#elif NETWORK_HAS_WINHTTP
  CORE_UNUSED(max_requests_count);
  return std::make_unique<NetworkWinHttp>();
#else 
  CORE_UNUSED(max_requests_count);
  static_assert(false, "No default network implementation provided");
#endif
}

} // namespace http
} // namespace olp
