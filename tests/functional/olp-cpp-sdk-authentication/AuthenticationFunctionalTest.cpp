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

#include "AuthenticationCommonTestFixture.h"

class AuthenticationFunctionalTest : public AuthenticationCommonTestFixture {};

TEST_F(AuthenticationFunctionalTest, SignInClient) {
  AuthenticationCredentials credentials(id_, secret_);
  std::time_t now;
  AuthenticationClient::SignInClientResponse response =
      SignInClient(credentials, now, kExpiryTime);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response.GetResult().GetStatus());
  EXPECT_STREQ(kErrorOk.c_str(),
               response.GetResult().GetErrorResponse().message.c_str());
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + kMaxExpiry, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinExpiry, response.GetResult().GetExpiryTime());
  EXPECT_FALSE(response.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());

  now = std::time(nullptr);
  AuthenticationClient::SignInClientResponse response_2 =
      SignInClient(credentials, now, kExtendedExpiryTime);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response_2.GetResult().GetStatus());
  EXPECT_FALSE(response_2.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + kMaxExtendedExpiry, response_2.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinExtendedExpiry, response_2.GetResult().GetExpiryTime());
  EXPECT_FALSE(response_2.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response_2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_2.GetResult().GetUserIdentifier().empty());

  now = std::time(nullptr);
  AuthenticationClient::SignInClientResponse response_3 =
      SignInClient(credentials, now, kCustomExpiryTime);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response_3.GetResult().GetStatus());
  EXPECT_FALSE(response_3.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + kMaxCustomExpiry, response_3.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinCustomExpiry, response_3.GetResult().GetExpiryTime());
  EXPECT_FALSE(response_3.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response_3.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_3.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationFunctionalTest, SignInClientMaxExpiration) {
  // Test maximum token expiration 24 h
  AuthenticationCredentials credentials(id_, secret_);

  std::time_t now;
  AuthenticationClient::SignInClientResponse response =
      SignInClient(credentials, now);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response.GetResult().GetStatus());
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_STREQ(kErrorOk.c_str(),
               response.GetResult().GetErrorResponse().message.c_str());
  EXPECT_GE(now + kMaxLimitExpiry, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinLimitExpiry, response.GetResult().GetExpiryTime());

  // Test token expiration greater than 24h
  AuthenticationClient::SignInClientResponse response_2 =
      SignInClient(credentials, now, 90000);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response_2.GetResult().GetStatus());
  EXPECT_FALSE(response_2.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + kMaxLimitExpiry, response_2.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinLimitExpiry, response_2.GetResult().GetExpiryTime());
  EXPECT_FALSE(response_2.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response_2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_2.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationFunctionalTest, InvalidCredentials) {
  AuthenticationCredentials credentials(id_, id_);

  std::time_t now;
  AuthenticationClient::SignInClientResponse response =
      SignInClient(credentials, now);
  EXPECT_EQ(olp::http::HttpStatusCode::UNAUTHORIZED,
            response.GetResult().GetStatus());
  EXPECT_EQ(kErrorUnauthorizedCode,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorUnauthorizedMessage,
            response.GetResult().GetErrorResponse().message);
  EXPECT_TRUE(response.GetResult().GetAccessToken().empty());
  EXPECT_TRUE(response.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationFunctionalTest, SignInClientCancel) {
  AuthenticationCredentials credentials(id_, secret_);

  std::time_t now;
  AuthenticationClient::SignInClientResponse response =
      SignInClient(credentials, now, kLimitExpiry, true);

  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(AuthenticationFunctionalTest, SignUpInUser) {
  const std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse signup_response = SignUpUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::CREATED,
            signup_response.GetResult().GetStatus());
  EXPECT_EQ(kErrorSignUpCreated,
            signup_response.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(signup_response.GetResult().GetUserIdentifier().empty());

  AuthenticationClient::SignInUserResponse response = SignInUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::PRECONDITION_FAILED,
            response.GetResult().GetStatus());
  EXPECT_EQ(kErrorPreconditionFailedCode,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorPreconditionFailedMessage,
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

  AuthenticationClient::SignInUserResponse response3 = SignInUser(email);
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

  AuthenticationUtils::DeleteUserResponse response4 =
      DeleteUser(response3.GetResult().GetAccessToken());
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT, response4.status);
  EXPECT_STREQ(kErrorNoContent.c_str(), response4.error.c_str());

  AuthenticationClient::SignInUserResponse response5 = SignInUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::UNAUTHORIZED,
            response5.GetResult().GetStatus());
  EXPECT_EQ(kErrorAccountNotFoundCode,
            response5.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorAccountNotFoundMessage,
            response5.GetResult().GetErrorResponse().message);
}

TEST_F(AuthenticationFunctionalTest, SignUpUserCancel) {
  const std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse response =
      SignUpUser(email, "password123", true);
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(AuthenticationFunctionalTest, SignInUserCancel) {
  const std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse signup_response = SignUpUser(email);
  EXPECT_TRUE(signup_response.IsSuccessful());

  AuthenticationClient::SignInUserResponse response = SignInUser(email, true);
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(AuthenticationFunctionalTest, AcceptTermCancel) {
  const std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse signup_response = SignUpUser(email);
  EXPECT_TRUE(signup_response.IsSuccessful());

  AuthenticationClient::SignInUserResponse response = SignInUser(email);
  EXPECT_TRUE(response.IsSuccessful());

  AuthenticationClient::SignInUserResponse response2 =
      AcceptTerms(response, true);
  EXPECT_FALSE(response2.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response2.GetError().GetErrorCode());

  AuthenticationClient::SignInUserResponse response3 = SignInUser(email);
  EXPECT_TRUE(response.IsSuccessful());

  AuthenticationClient::SignOutUserResponse signOutResponse =
      SignOutUser(response3.GetResult().GetAccessToken());
  EXPECT_FALSE(response2.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response2.GetError().GetErrorCode());

  AuthenticationUtils::DeleteUserResponse response4 =
      DeleteUser(response3.GetResult().GetAccessToken());
}

TEST_F(AuthenticationFunctionalTest, SignInRefresh) {
  const std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse signup_response = SignUpUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::CREATED,
            signup_response.GetResult().GetStatus());
  EXPECT_EQ(kErrorSignUpCreated,
            signup_response.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(signup_response.GetResult().GetUserIdentifier().empty());

  AuthenticationClient::SignInUserResponse response = SignInUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::PRECONDITION_FAILED,
            response.GetResult().GetStatus());
  EXPECT_EQ(kErrorPreconditionFailedCode,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorPreconditionFailedMessage,
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

  AuthenticationClient::SignInUserResponse response3 = SignInUser(email);
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

  AuthenticationClient::SignInUserResponse response4 =
      SignInRefesh(response3.GetResult().GetAccessToken(),
                   response3.GetResult().GetRefreshToken());
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response4.GetResult().GetStatus());
  EXPECT_EQ(kErrorOk, response4.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(response4.GetResult().GetAccessToken().empty());
  EXPECT_FALSE(response4.GetResult().GetTokenType().empty());
  EXPECT_FALSE(response4.GetResult().GetRefreshToken().empty());
  EXPECT_FALSE(response4.GetResult().GetUserIdentifier().empty());
  EXPECT_TRUE(response4.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_TRUE(response4.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_TRUE(response4.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_TRUE(response4.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_TRUE(response4.GetResult().GetPrivatePolicyUrlJson().empty());

  AuthenticationClient::SignInUserResponse response5 =
      SignInRefesh("12345", response3.GetResult().GetRefreshToken());
  EXPECT_EQ(olp::http::HttpStatusCode::UNAUTHORIZED,
            response5.GetResult().GetStatus());
  EXPECT_EQ(kErrorRefreshFailedCode,
            response5.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorRefreshFailedMessage,
            response5.GetResult().GetErrorResponse().message);

  AuthenticationUtils::DeleteUserResponse response6 =
      DeleteUser(response4.GetResult().GetAccessToken());
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT, response6.status);
  EXPECT_STREQ(kErrorNoContent.c_str(), response6.error.c_str());

  AuthenticationClient::SignInUserResponse response7 = SignInUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::UNAUTHORIZED,
            response7.GetResult().GetStatus());
  EXPECT_EQ(kErrorAccountNotFoundCode,
            response7.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorAccountNotFoundMessage,
            response7.GetResult().GetErrorResponse().message);
}

TEST_F(AuthenticationFunctionalTest, SignInRefreshCancel) {
  const std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse signup_response = SignUpUser(email);
  EXPECT_TRUE(signup_response.IsSuccessful());

  AuthenticationClient::SignInUserResponse response = SignInUser(email);
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::PRECONDITION_FAILED,
            response.GetResult().GetStatus());

  AuthenticationClient::SignInUserResponse response2 = AcceptTerms(response);
  EXPECT_TRUE(response2.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT,
            response2.GetResult().GetStatus());

  AuthenticationClient::SignInUserResponse response3 = SignInUser(email);
  EXPECT_TRUE(response3.IsSuccessful());

  AuthenticationClient::SignInUserResponse response4 =
      SignInRefesh(response3.GetResult().GetAccessToken(),
                   response3.GetResult().GetRefreshToken(), true);
  EXPECT_FALSE(response4.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response4.GetError().GetErrorCode());

  AuthenticationUtils::DeleteUserResponse response5 =
      DeleteUser(response3.GetResult().GetAccessToken());
}

TEST_F(AuthenticationFunctionalTest, SignOutUser) {
  const std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse signup_response = SignUpUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::CREATED,
            signup_response.GetResult().GetStatus());
  EXPECT_EQ(kErrorSignUpCreated,
            signup_response.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(signup_response.GetResult().GetUserIdentifier().empty());

  AuthenticationClient::SignInUserResponse response = SignInUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::PRECONDITION_FAILED,
            response.GetResult().GetStatus());
  EXPECT_EQ(kErrorPreconditionFailedCode,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorPreconditionFailedMessage,
            response.GetResult().GetErrorResponse().message);

  AuthenticationClient::SignInUserResponse response2 = AcceptTerms(response);
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT,
            response2.GetResult().GetStatus());
  EXPECT_STREQ(kErrorNoContent.c_str(),
               response2.GetResult().GetErrorResponse().message.c_str());

  AuthenticationClient::SignInUserResponse response3 = SignInUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response3.GetResult().GetStatus());
  EXPECT_STREQ(kErrorOk.c_str(),
               response3.GetResult().GetErrorResponse().message.c_str());

  AuthenticationClient::SignOutUserResponse signOutResponse =
      SignOutUser(response3.GetResult().GetAccessToken());
  EXPECT_TRUE(signOutResponse.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT,
            signOutResponse.GetResult().GetStatus());
  EXPECT_EQ(kErrorNoContent,
            signOutResponse.GetResult().GetErrorResponse().message);

  AuthenticationUtils::DeleteUserResponse response4 =
      DeleteUser(response3.GetResult().GetAccessToken());
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT, response4.status);
  EXPECT_STREQ(kErrorNoContent.c_str(), response4.error.c_str());
}

TEST_F(AuthenticationFunctionalTest, NetworkProxySettings) {
  AuthenticationCredentials credentials(id_, secret_);

  auto proxy_settings = olp::http::NetworkProxySettings();
  client_->SetNetworkProxySettings(proxy_settings);
  proxy_settings.WithHostname("$.?");
  proxy_settings.WithPort(42);
  proxy_settings.WithType(olp::http::NetworkProxySettings::Type::SOCKS4);

  client_->SetNetworkProxySettings(proxy_settings);
  std::time_t now;
  auto response = SignInClient(credentials, now, kExpiryTime);
  // Bad proxy error code and message varies by platform
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_TRUE(response.GetError().GetErrorCode() ==
              olp::client::ErrorCode::ServiceUnavailable);
  EXPECT_STRNE(response.GetError().GetMessage().c_str(), kErrorOk.c_str());
}

TEST_F(AuthenticationFunctionalTest, ErrorFields) {
  static const std::string PASSWORD = "password";
  static const std::string EMAIL = "email";

  AuthenticationClient::SignUpResponse signup_response =
      SignUpUser("a/*<@test.com", "password");
  EXPECT_TRUE(signup_response.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
            signup_response.GetResult().GetStatus());
  EXPECT_EQ(kErrorFieldsCode,
            signup_response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorFieldsMessage,
            signup_response.GetResult().GetErrorResponse().message);
  EXPECT_EQ(2, signup_response.GetResult().GetErrorFields().size());
  int count = 0;
  for (ErrorFields::const_iterator it =
           signup_response.GetResult().GetErrorFields().begin();
       it != signup_response.GetResult().GetErrorFields().end(); it++) {
    const std::string name = count == 0 ? EMAIL : PASSWORD;
    const std::string message =
        count == 0 ? kErrorIllegalEmail : kErrorBlacklistedPassword;
    unsigned int code =
        count == 0 ? kErrorIllegalEmailCode : kErrorBlacklistedPasswordCode;
    count++;
    EXPECT_EQ(name, it->name);
    EXPECT_EQ(message, it->message);
    EXPECT_EQ(code, it->code);
  }
}
