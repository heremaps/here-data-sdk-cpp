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

#include "AuthenticationOnlineTest.h"

class FacebookAuthenticationOnlineTest : public AuthenticationOnlineTest {
 public:
  void SetUp() override {
    AuthenticationOnlineTest::SetUp();
    facebook_ = std::make_unique<FacebookTestUtils>();
    ASSERT_TRUE(facebook_->CreateFacebookTestUser(
        *network_, olp::http::NetworkSettings(), test_user, "email"));
    id_ = TEST_APP_KEY_ID;
    secret_ = TEST_APP_KEY_SECRET;
  }

  AuthenticationClient::SignInUserResponse SignInFacebook(
      std::string token = "") {
    AuthenticationCredentials credentials(id_, secret_);
    std::promise<AuthenticationClient::SignInUserResponse> request;
    auto request_future = request.get_future();
    AuthenticationClient::FederatedProperties properties;
    properties.access_token = token.empty() ? test_user.access_token : token;
    properties.country_code = "usa";
    properties.language = "en";
    properties.email = TEST_USER_NAME + "@example.com";
    client_->SignInFacebook(
        credentials, properties,
        [&](const AuthenticationClient::SignInUserResponse& response) {
          request.set_value(response);
        });
    request_future.wait();
    return request_future.get();
  }

  void DeleteFacebookTestUser(
      olp::http::Network& network,
      const olp::http::NetworkSettings& network_settings,
      const std::string& id) {
    for (int retry = 0; retry < 3; ++retry) {
      if (facebook_->DeleteFacebookTestUser(network, network_settings, id)) {
        return;
      }

      std::this_thread::sleep_for(std::chrono::seconds(retry));
    }
  }

  void TearDown() override {
    DeleteFacebookTestUser(*network_, olp::http::NetworkSettings(),
                           test_user.id);
    AuthenticationOnlineTest::TearDown();
    facebook_.reset();
  }

 private:
  std::unique_ptr<FacebookTestUtils> facebook_;
  FacebookTestUtils::FacebookUser test_user;
};
