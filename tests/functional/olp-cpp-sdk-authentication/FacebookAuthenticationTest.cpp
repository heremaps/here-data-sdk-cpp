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

#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include "AuthenticationCommonTestFixture.h"
#include "AuthenticationTestUtils.h"
#include "TestConstants.h"

using namespace ::olp::authentication;

namespace {

static const int kErrorFacebookErrorCode = 400300;

const std::string kErrorFacebookFailedMessage = "Unexpected Facebook error.";

class FacebookAuthenticationTest : public AuthenticationCommonTestFixture {
 protected:
  void SetUp() override {
    AuthenticationCommonTestFixture::SetUp();

    ASSERT_TRUE(AuthenticationTestUtils::CreateFacebookTestUser(
        *network_, olp::http::NetworkSettings(), test_user_, "email"));

    id_ = kTestAppKeyId;
    secret_ = kTestAppKeySecret;
  }

  void TearDown() override {
    DeleteFacebookTestUser(*network_, olp::http::NetworkSettings(),
                           test_user_.id);

    test_user_ = AuthenticationTestUtils::FacebookUser{};
    AuthenticationCommonTestFixture::TearDown();
  }

  AuthenticationClient::SignInUserResponse SignInFacebook(
      std::string token = "") {
    AuthenticationCredentials credentials(id_, secret_);
    std::promise<AuthenticationClient::SignInUserResponse> request;
    auto request_future = request.get_future();
    AuthenticationClient::FederatedProperties properties;
    properties.access_token =
        token.empty() ? test_user_.token.access_token : token;
    properties.country_code = "usa";
    properties.language = "en";
    properties.email = kTestUserName + "@example.com";

    client_->SignInFacebook(
        credentials, properties,
        [&](const AuthenticationClient::SignInUserResponse& response) {
          request.set_value(response);
        });
    return request_future.get();
  }

  void DeleteFacebookTestUser(
      olp::http::Network& network,
      const olp::http::NetworkSettings& network_settings,
      const std::string& id) {
    for (int retry = 0; retry < 3; ++retry) {
      if (AuthenticationTestUtils::DeleteFacebookTestUser(
              network, network_settings, id)) {
        return;
      }

      std::this_thread::sleep_for(std::chrono::seconds(retry));
    }
  }

 private:
  AuthenticationTestUtils::FacebookUser test_user_;
};

TEST_F(FacebookAuthenticationTest, SignInFacebook) {
  AuthenticationClient::SignInUserResponse response = SignInFacebook();
  EXPECT_EQ(olp::http::HttpStatusCode::CREATED,
            response.GetResult().GetStatus());
  EXPECT_EQ(kErrorPreconditionCreatedCode,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorPreconditionCreatedMessage,
            response.GetResult().GetErrorResponse().message);
  EXPECT_TRUE(response.GetResult().GetAccessToken().empty());
  EXPECT_TRUE(response.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());
  EXPECT_FALSE(response.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_FALSE(response.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_FALSE(response.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_FALSE(response.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_FALSE(response.GetResult().GetPrivatePolicyUrlJson().empty());

  AuthenticationClient::SignInUserResponse response2 = AcceptTerms(response);
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT,
            response2.GetResult().GetStatus());
  EXPECT_EQ(kErrorNoContent, response2.GetResult().GetErrorResponse().message);
  EXPECT_TRUE(response2.GetResult().GetAccessToken().empty());
  EXPECT_TRUE(response2.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response2.GetResult().GetUserIdentifier().empty());
  EXPECT_TRUE(response2.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_TRUE(response2.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_TRUE(response2.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_TRUE(response2.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_TRUE(response2.GetResult().GetPrivatePolicyUrlJson().empty());

  AuthenticationClient::SignInUserResponse response3 = SignInFacebook();
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response3.GetResult().GetStatus());
  EXPECT_EQ(kErrorOk, response3.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(response3.GetResult().GetAccessToken().empty());
  EXPECT_FALSE(response3.GetResult().GetTokenType().empty());
  EXPECT_FALSE(response3.GetResult().GetRefreshToken().empty());
  EXPECT_FALSE(response3.GetResult().GetUserIdentifier().empty());
  EXPECT_TRUE(response3.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_TRUE(response3.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_TRUE(response3.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_TRUE(response3.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_TRUE(response3.GetResult().GetPrivatePolicyUrlJson().empty());

  AuthenticationTestUtils::DeleteUserResponse response4 =
      DeleteUser(response3.GetResult().GetAccessToken());
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT, response4.status);
  EXPECT_STREQ(kErrorNoContent.c_str(), response4.error.c_str());

  // SignIn with invalid token
  AuthenticationClient::SignInUserResponse response5 = SignInFacebook("12345");
  EXPECT_EQ(olp::http::HttpStatusCode::UNAUTHORIZED,
            response5.GetResult().GetStatus());
  EXPECT_EQ(kErrorFacebookErrorCode,
            response5.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorFacebookFailedMessage,
            response5.GetResult().GetErrorResponse().message);
  EXPECT_TRUE(response5.GetResult().GetAccessToken().empty());
  EXPECT_TRUE(response5.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response5.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response5.GetResult().GetUserIdentifier().empty());
  EXPECT_TRUE(response5.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_TRUE(response5.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_TRUE(response5.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_TRUE(response5.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_TRUE(response5.GetResult().GetPrivatePolicyUrlJson().empty());
}

}  // namespace
