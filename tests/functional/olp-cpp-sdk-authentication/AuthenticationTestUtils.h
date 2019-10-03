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

#include <string>
#include <vector>
#include <memory>

#include <olp/core/http/Network.h>

struct AccessTokenOutcome {
  std::string access_token;
  int status;
};

struct FacebookUser {
  AccessTokenOutcome token;
  std::string id;
};

class AuthenticationTestUtils final {
 public:
  static bool CreateFacebookTestUser(
      olp::http::Network& network,
      const olp::http::NetworkSettings& network_settings,
      const std::string& permissions, FacebookUser& user);
  static bool DeleteFacebookTestUser(
      olp::http::Network& network,
      const olp::http::NetworkSettings& network_settings,
      const std::string& user_id);

  static bool GetGoogleAccessToken(
      olp::http::Network& network,
      const olp::http::NetworkSettings& network_settings,
      AccessTokenOutcome& token);

  static bool GetArcGisAccessToken(
      olp::http::Network& network,
      const olp::http::NetworkSettings& network_settings,
      AccessTokenOutcome& token);

 private:
  static std::shared_ptr<std::vector<unsigned char>> GenerateArcGisClientBody();

  static bool GetAccessTokenImpl(olp::http::Network& network,
                                 const olp::http::NetworkRequest& request,
                                 AccessTokenOutcome& token);
  AuthenticationTestUtils() = delete;
};
