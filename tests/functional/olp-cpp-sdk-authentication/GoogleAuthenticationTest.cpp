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
#include "ErrorCodes.h"
#include "ErrorMessages.h"
#include "TestConstants.h"

using namespace ::olp::authentication;

class GoogleAuthenticationTest : public AuthenticationCommonTestFixture {
 protected:
  void SetUp() override {
    AuthenticationCommonTestFixture::SetUp();

    ASSERT_TRUE(AuthenticationTestUtils::GetGoogleAccessToken(
        *network_, olp::http::NetworkSettings(), token_));
  }

  void TearDown() override {
    token_ = AccessTokenOutcome{};
    AuthenticationCommonTestFixture::TearDown();
  }

  AuthenticationClient::SignInUserResponse SignInGoogleUser(
      const std::string& email, const std::string& access_token) {
    AuthenticationCredentials credentials(GetAppKey(), GetAppSecretKey());
    std::promise<AuthenticationClient::SignInUserResponse> request;
    auto request_future = request.get_future();
    AuthenticationClient::FederatedProperties properties;
    properties.access_token = access_token;
    properties.country_code = "USA";
    properties.language = "en";
    properties.email = email;
    client_->SignInGoogle(
        credentials, properties,
        [&](const AuthenticationClient::SignInUserResponse& response) {
          request.set_value(response);
        });
    request_future.wait();
    return request_future.get();
  }

  std::string GetAppKey() const override { return kTestAppKeyId; }

  std::string GetAppSecretKey() const override { return kTestAppKeySecret; }

 protected:
  AccessTokenOutcome token_;
};

TEST_F(GoogleAuthenticationTest, SignInGoogle) {
  ASSERT_FALSE(token_.access_token.empty());

  const std::string email = GetEmail();
  OLP_SDK_LOG_TRACE("GoogleAuthenticationTest",
                    "Running test with e-mail=" << email);

  AuthenticationClient::SignInUserResponse response =
      SignInGoogleUser(email, token_.access_token);
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

  OLP_SDK_LOG_TRACE(
      "GoogleAuthenticationTest",
      "termAcceptanceToken=" << response.GetResult().GetTermAcceptanceToken());

  AuthenticationClient::SignInUserResponse response2 = AcceptTerms(response);
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT,
            response2.GetResult().GetStatus());
  EXPECT_STREQ(kErrorNoContent.c_str(),
               response2.GetResult().GetErrorResponse().message.c_str());
  EXPECT_TRUE(response2.GetResult().GetAccessToken().empty());
  EXPECT_TRUE(response2.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response2.GetResult().GetUserIdentifier().empty());
  EXPECT_TRUE(response2.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_TRUE(response2.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_TRUE(response2.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_TRUE(response2.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_TRUE(response2.GetResult().GetPrivatePolicyUrlJson().empty());

  AuthenticationClient::SignInUserResponse response3 =
      SignInGoogleUser(email, token_.access_token);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response3.GetResult().GetStatus());
  EXPECT_STREQ(kErrorOk.c_str(),
               response3.GetResult().GetErrorResponse().message.c_str());
  EXPECT_FALSE(response3.GetResult().GetAccessToken().empty());
  EXPECT_FALSE(response3.GetResult().GetTokenType().empty());
  EXPECT_FALSE(response3.GetResult().GetRefreshToken().empty());
  EXPECT_FALSE(response3.GetResult().GetUserIdentifier().empty());
  EXPECT_TRUE(response3.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_TRUE(response3.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_TRUE(response3.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_TRUE(response3.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_TRUE(response3.GetResult().GetPrivatePolicyUrlJson().empty());

  AuthenticationClient::SignOutUserResponse signout_response =
      SignOutUser(response3.GetResult().GetAccessToken());
  EXPECT_TRUE(signout_response.IsSuccessful());
  // EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT,
  // signOutResponse.GetResult().GetStatus());
  // EXPECT_EQ(kErrorNoContent.c_str(),
  //          signOutResponse.GetResult().GetErrorResponse().message);

  DeleteUserResponse response4 =
      DeleteUser(response3.GetResult().GetAccessToken());
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT, response4.status);
  EXPECT_STREQ(kErrorNoContent.c_str(), response4.error.c_str());

  // SignIn with invalid token
  AuthenticationClient::SignInUserResponse response5 =
      SignInGoogleUser(email, "12345");
  EXPECT_EQ(olp::http::HttpStatusCode::UNAUTHORIZED,
            response5.GetResult().GetStatus());
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
