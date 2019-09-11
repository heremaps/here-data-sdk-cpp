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

#include <memory>
#include <string>

namespace olp {

namespace http {
class Network;
class NetworkSettings;
}  // namespace http

namespace authentication {
class FacebookTestUtils {
 public:
  struct FacebookUser {
    std::string access_token;
    std::string id;
    int status;
  };

  FacebookTestUtils();
  virtual ~FacebookTestUtils();
  bool createFacebookTestUser(
      olp::http::Network& network,
      const olp::http::NetworkSettings& network_settings, FacebookUser& user,
      const std::string& permissions);
  bool deleteFacebookTestUser(
      http::Network& network,
      const olp::http::NetworkSettings& network_settings, std::string user_id);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace authentication
}  // namespace olp
