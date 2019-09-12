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
#include <olp/core/http/Network.h>
#include <olp/core/http/NetworkRequest.h>
#include <olp/core/http/NetworkResponse.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/utils/Url.h>

#include <chrono>
#include <sstream>

namespace {
constexpr auto HYPE_DEV_ENV_PARTITION_HRN = "here-dev";
constexpr auto HYPE_PROD_ENV_PARTITION_HRN = "here";

const std::map<std::string, std::string> authentication_server_url = {
    {HYPE_DEV_ENV_PARTITION_HRN, "https://stg.account.api.here.com"},
    {HYPE_PROD_ENV_PARTITION_HRN, "https://account.api.here.com"}};

// Tags
constexpr auto kAuthorization = "Authorization";
constexpr auto kContentType = "Content-Type";
constexpr auto kApplicationJson = "application/json";
constexpr auto kDeleteUserEndpoint = "/user/me";

}  // namespace

namespace olp {
namespace authentication {
class AuthenticationUtils::Impl {
 public:
  /**
   * @brief Constructor
   * @param token_cache_limit Maximum number of tokens that will be cached.
   */
  Impl();

  virtual ~Impl() = default;

  void deleteHereUser(http::Network& network,
                      const http::NetworkSettings& network_settings,
                      const std::string& user_bearer_token,
                      const UserCallback& callback);

 private:
  std::string base64Encode(const std::vector<uint8_t>& vector);

  std::string generateBearerHeader(const std::string& user_bearer_token);

  std::string generateUid(size_t length = 32u, size_t groupLength = 8u);
};

AuthenticationUtils::Impl::Impl() {
  srand(static_cast<unsigned int>(
      std::chrono::system_clock::now().time_since_epoch().count()));
}

void AuthenticationUtils::Impl::deleteHereUser(
    olp::http::Network& network, const http::NetworkSettings& network_settings,
    const std::string& user_bearer_token, const UserCallback& callback) {
  std::string url = authentication_server_url.at(HYPE_DEV_ENV_PARTITION_HRN);
  url.append(kDeleteUserEndpoint);

  olp::http::NetworkRequest request(url);
  request.WithVerb(http::NetworkRequest::HttpVerb::DEL);
  request.WithHeader(kAuthorization, generateBearerHeader(user_bearer_token));
  request.WithHeader(kContentType, kApplicationJson);
  request.WithSettings(network_settings);

  std::shared_ptr<std::stringstream> payload =
      std::make_shared<std::stringstream>();
  network.Send(
      request, payload,
      [callback, payload](const olp::http::NetworkResponse& network_response) {
        DeleteUserResponse response;
        response.status = network_response.GetStatus();
        response.error = network_response.GetError();
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

void AuthenticationUtils::deleteHereUser(
    http::Network& network, const http::NetworkSettings& network_settings,
    const std::string& user_bearer_token, const UserCallback& callback) {
  d->deleteHereUser(network, network_settings, user_bearer_token, callback);
}

}  // namespace authentication
}  // namespace olp
