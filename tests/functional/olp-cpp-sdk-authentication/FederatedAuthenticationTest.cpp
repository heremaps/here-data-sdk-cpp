/*
 * Copyright (C) 2020-2025 HERE Europe B.V.
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
#include <olp/core/porting/make_unique.h>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>

#include "AuthenticationCommonTestFixture.h"
#include "AuthenticationTestUtils.h"
#include "TestConstants.h"

using namespace ::olp::authentication;

namespace {

class FederatedAuthenticationTest : public AuthenticationCommonTestFixture {
 protected:
  void SetUp() override {
    AuthenticationCommonTestFixture::SetUp();

    ASSERT_TRUE(AuthenticationTestUtils::GetGoogleAccessToken(
        *network_, olp::http::NetworkSettings(), token_));

    id_ = kTestAppKeyId;
    secret_ = kTestAppKeySecret;
  }

  void TearDown() override {
    token_ = AuthenticationTestUtils::AccessTokenResponse{};

    AuthenticationCommonTestFixture::TearDown();
  }

  AuthenticationClient::SignInUserResponse SignInFederated(std::string body) {
    AuthenticationCredentials credentials(id_, secret_);
    std::promise<AuthenticationClient::SignInUserResponse> request;
    auto request_future = request.get_future();

    client_->SignInFederated(
        credentials, body,
        [&](const AuthenticationClient::SignInUserResponse& response) {
          request.set_value(response);
        });

    return request_future.get();
  }

  std::string GoogleAuthenticationBody(const std::string& email,
                                       const std::string& access_token) {
    boost::json::object object;
    object.emplace("grantType", "google");
    object.emplace("accessToken", access_token);
    object.emplace("countryCode", "USA");
    object.emplace("language", "en");
    object.emplace("email", email);

    return boost::json::serialize(object);
  }

 protected:
  AuthenticationTestUtils::AccessTokenResponse token_;
};

TEST_F(FederatedAuthenticationTest, SignInFederatedNoBody) {
  AuthenticationClient::SignInUserResponse response = SignInFederated("");
  EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
            response.GetResult().GetStatus());
  EXPECT_TRUE(response.GetResult().GetAccessToken().empty());
  EXPECT_TRUE(response.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());
  EXPECT_TRUE(response.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_TRUE(response.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_TRUE(response.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_TRUE(response.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_TRUE(response.GetResult().GetPrivatePolicyUrlJson().empty());
}

TEST_F(FederatedAuthenticationTest, SignInFederatedEmptyJson) {
  AuthenticationClient::SignInUserResponse response = SignInFederated("{}");
  EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
            response.GetResult().GetStatus());
  EXPECT_EQ(kErrorFieldsCode, response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorFieldsMessage,
            response.GetResult().GetErrorResponse().message);
  EXPECT_TRUE(response.GetResult().GetAccessToken().empty());
  EXPECT_TRUE(response.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());
  EXPECT_TRUE(response.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_TRUE(response.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_TRUE(response.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_TRUE(response.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_TRUE(response.GetResult().GetPrivatePolicyUrlJson().empty());
}

TEST_F(FederatedAuthenticationTest, SignInGoogle) {
  ASSERT_FALSE(token_.access_token.empty());

  const std::string email = GetEmail();

  {
    SCOPED_TRACE("Create account");
    AuthenticationClient::SignInUserResponse response =
        SignInFederated(GoogleAuthenticationBody(email, token_.access_token));
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

    SCOPED_TRACE("Accept Terms");
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
  }

  {
    SCOPED_TRACE("Sign in");
    AuthenticationClient::SignInUserResponse response3 =
        SignInFederated(GoogleAuthenticationBody(email, token_.access_token));
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

    SCOPED_TRACE("Sign out");
    AuthenticationClient::SignOutUserResponse signout_response =
        SignOutUser(response3.GetResult().GetAccessToken());
    EXPECT_TRUE(signout_response.IsSuccessful());

    SCOPED_TRACE("Delete account");
    AuthenticationTestUtils::DeleteUserResponse response4 =
        DeleteUser(response3.GetResult().GetAccessToken());
    EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT, response4.status);
    EXPECT_STREQ(kErrorNoContent.c_str(), response4.error.c_str());
  }

  {
    SCOPED_TRACE("Sign in with invalid token");
    // SignIn with invalid token
    AuthenticationClient::SignInUserResponse response5 =
        SignInFederated(GoogleAuthenticationBody(email, "12345"));
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
}

}  // namespace
