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

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace olp {
namespace http {
class Network;
class NetworkSettings;
class NetworkRequest;
}  // network
}  // olp

class AuthenticationTestUtils final {
 public:
  struct AccessTokenResponse {
    std::string access_token;
    int status;
  };

  struct FacebookUser {
    AccessTokenResponse token;
    std::string id;
  };

  struct DeleteUserResponse {
    int status;
    std::string error;
  };

  using DeleteHereUserCallback =
      std::function<void(const DeleteUserResponse& response)>;

 public:
  static bool CreateFacebookTestUser(
      olp::http::Network& network,
      const olp::http::NetworkSettings& network_settings, FacebookUser& user,
      const std::string& permissions);
  static bool DeleteFacebookTestUser(
      olp::http::Network& network,
      const olp::http::NetworkSettings& network_settings,
      const std::string& user_id);

  static bool GetGoogleAccessToken(
      olp::http::Network& network,
      const olp::http::NetworkSettings& network_settings,
      AccessTokenResponse& token);

  static bool GetArcGisAccessToken(
      olp::http::Network& network,
      const olp::http::NetworkSettings& network_settings,
      AccessTokenResponse& token);

  static void DeleteHereUser(olp::http::Network& network,
                             const olp::http::NetworkSettings& network_settings,
                             const std::string& user_bearer_token,
                             const DeleteHereUserCallback& callback);

 private:
  static std::shared_ptr<std::vector<unsigned char>> GenerateArcGisClientBody();

  static bool GetAccessTokenImpl(olp::http::Network& network,
                                 const olp::http::NetworkRequest& request,
                                 AccessTokenResponse& token);

  static std::string GenerateBearerHeader(const std::string& user_bearer_token);

  AuthenticationTestUtils() = delete;
};
