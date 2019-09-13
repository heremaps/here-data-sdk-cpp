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

class ArcGisAuthenticationOnlineTest : public AuthenticationOnlineTest {
 public:
  void SetUp() override {
    AuthenticationOnlineTest::SetUp();
    arc_gis_ = std::make_unique<ArcGisTestUtils>();
    ASSERT_TRUE(arc_gis_->getAccessToken(
        *network_, olp::http::NetworkSettings(), test_user_));
    id_ = TEST_APP_KEY_ID;
    secret_ = TEST_APP_KEY_SECRET;
  }

  AuthenticationClient::SignInUserResponse SignInArcGis(
      const std::string& email, const std::string& token = "") {
    AuthenticationCredentials credentials(id_, secret_);
    std::promise<AuthenticationClient::SignInUserResponse> request;
    auto request_future = request.get_future();
    AuthenticationClient::FederatedProperties properties;
    properties.access_token = token.empty() ? test_user_.access_token : token;
    properties.country_code = "usa";
    properties.language = "en";
    properties.email = email;
    client_->SignInArcGis(
        credentials, properties,
        [&](const AuthenticationClient::SignInUserResponse& response) {
          request.set_value(response);
        });
    request_future.wait();
    return request_future.get();
  }

  void TearDown() override {
    AuthenticationOnlineTest::TearDown();
    arc_gis_.reset();
  }

 private:
  std::unique_ptr<ArcGisTestUtils> arc_gis_;
  ArcGisTestUtils::ArcGisUser test_user_;
};
