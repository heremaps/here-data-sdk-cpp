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

#include "AuthenticationUtils.h"
#include <olp/core/network/Network.h>
#include <olp/core/network/NetworkRequest.h>
#include <olp/core/network/NetworkResponse.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/utils/Url.h>
#include <sstream>

#define HYPE_DEV_ENV_PARTITION_HRN "here-dev"
#define HYPE_PROD_ENV_PARTITION_HRN "here"

const std::map<std::string, std::string> authentication_server_url = {
    {HYPE_DEV_ENV_PARTITION_HRN, "https://stg.account.api.here.com"},
    {HYPE_PROD_ENV_PARTITION_HRN, "https://account.api.here.com"}};

using namespace std;
using namespace olp::network;

// Tags
static const std::string AUTHORIZATION = "Authorization";
static const std::string CONTENT_TYPE = "Content-Type";
static const std::string APPLICATION_JSON = "application/json";
static const std::string DELETE_USER_ENDPOINT = "/user/me";

namespace olp {
namespace authentication {
class AuthenticationUtils::Impl {
 public:
  class ScopedNetwork {
   public:
    ScopedNetwork() : network() { network.Start(); }

    Network& getNetwork() { return network; }

   private:
    Network network;
  };

  using ScopedNetworkPtr = std::shared_ptr<ScopedNetwork>;
  /**
   * @brief Constructor
   * @param token_cache_limit Maximum number of tokens that will be cached.
   */
  Impl();

  virtual ~Impl() = default;

  void deleteHereUser(const std::string& user_bearer_token,
                      const UserCallback& callback);

 private:
  std::string base64Encode(const std::vector<uint8_t>& vector);

  std::string generateBearerHeader(const std::string& user_bearer_token);

  std::string generateUid(size_t length = 32u, size_t groupLength = 8u);

  ScopedNetworkPtr getScopedNetwork();

 private:
  std::weak_ptr<ScopedNetwork> networkPtr;
  std::mutex networkPtrLock;
};

AuthenticationUtils::Impl::ScopedNetworkPtr
AuthenticationUtils::Impl::getScopedNetwork() {
  std::lock_guard<std::mutex> lock(networkPtrLock);
  auto netPtr = networkPtr.lock();
  if (netPtr) return netPtr;

  auto result = std::make_shared<ScopedNetwork>();
  networkPtr = result;
  return result;
}

AuthenticationUtils::Impl::Impl() {
  srand((unsigned int)std::chrono::system_clock::now()
            .time_since_epoch()
            .count());
}

void AuthenticationUtils::Impl::deleteHereUser(
    const std::string& user_bearer_token, const UserCallback& callback) {
  std::string url = authentication_server_url.at(HYPE_DEV_ENV_PARTITION_HRN);
  url.append(DELETE_USER_ENDPOINT);
  NetworkRequest request(url, 0, NetworkRequest::PriorityDefault,
                         NetworkRequest::HttpVerb::DEL);
  request.AddHeader(AUTHORIZATION, generateBearerHeader(user_bearer_token));
  request.AddHeader(CONTENT_TYPE, APPLICATION_JSON);

  std::shared_ptr<std::stringstream> payload =
      std::make_shared<std::stringstream>();
  auto networkPtr = getScopedNetwork();
  networkPtr->getNetwork().Send(
      request, payload,
      [networkPtr, callback, payload](const NetworkResponse& network_response) {
        DeleteUserResponse response;
        response.status = network_response.Status();
        response.error = network_response.Error();
        callback(response);
      });
}

std::string AuthenticationUtils::Impl::generateBearerHeader(
    const std::string& user_bearer_token) {
  std::string authorization = "Bearer ";
  authorization += user_bearer_token;
  return authorization;
}

AuthenticationUtils::AuthenticationUtils() : d(std::make_unique<Impl>()) {}

AuthenticationUtils::~AuthenticationUtils() = default;

void AuthenticationUtils::deleteHereUser(const std::string& user_bearer_token,
                                         const UserCallback& callback) {
  d->deleteHereUser(user_bearer_token, callback);
}

}  // namespace authentication
}  // namespace olp
