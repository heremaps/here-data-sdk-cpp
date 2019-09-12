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

#include "olp/core/http/HttpStatusCode.h"

using namespace olp::authentication;

std::shared_ptr<olp::http::Network> AuthenticationOnlineTest::s_network_;

TEST_F(AuthenticationOnlineTest, SignInClient) {
  AuthenticationCredentials credentials(id_, secret_);
  std::time_t now;
  AuthenticationClient::SignInClientResponse response =
      SignInClient(credentials, now, EXPIRY_TIME);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response.GetResult().GetStatus());
  EXPECT_STREQ(ERROR_OK.c_str(),
               response.GetResult().GetErrorResponse().message.c_str());
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + MAX_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_FALSE(response.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());

  now = std::time(nullptr);
  AuthenticationClient::SignInClientResponse response_2 =
      SignInClient(credentials, now, EXTENDED_EXPIRY_TIME);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response_2.GetResult().GetStatus());
  EXPECT_FALSE(response_2.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + MAX_EXTENDED_EXPIRY, response_2.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_EXTENDED_EXPIRY, response_2.GetResult().GetExpiryTime());
  EXPECT_FALSE(response_2.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response_2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_2.GetResult().GetUserIdentifier().empty());

  now = std::time(nullptr);
  AuthenticationClient::SignInClientResponse response_3 =
      SignInClient(credentials, now, CUSTOM_EXPIRY_TIME);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response_3.GetResult().GetStatus());
  EXPECT_FALSE(response_3.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + MAX_CUSTOM_EXPIRY, response_3.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_CUSTOM_EXPIRY, response_3.GetResult().GetExpiryTime());
  EXPECT_FALSE(response_3.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response_3.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_3.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationOnlineTest, SignInClientMaxExpiration) {
  // Test maximum token expiration 24 h
  AuthenticationCredentials credentials(id_, secret_);

  std::time_t now;
  AuthenticationClient::SignInClientResponse response =
      SignInClient(credentials, now);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response.GetResult().GetStatus());
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_STREQ(ERROR_OK.c_str(),
               response.GetResult().GetErrorResponse().message.c_str());
  EXPECT_GE(now + MAX_LIMIT_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_LIMIT_EXPIRY, response.GetResult().GetExpiryTime());

  // Test token expiration greater than 24h
  AuthenticationClient::SignInClientResponse response_2 =
      SignInClient(credentials, now, 90000);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response_2.GetResult().GetStatus());
  EXPECT_FALSE(response_2.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + MAX_LIMIT_EXPIRY, response_2.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_LIMIT_EXPIRY, response_2.GetResult().GetExpiryTime());
  EXPECT_FALSE(response_2.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response_2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_2.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationOnlineTest, InvalidCredentials) {
  AuthenticationCredentials credentials(id_, id_);

  std::time_t now;
  AuthenticationClient::SignInClientResponse response =
      SignInClient(credentials, now);
  EXPECT_EQ(olp::http::HttpStatusCode::UNAUTHORIZED,
            response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_UNAUTHORIZED_CODE,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_UNAUTHORIZED_MESSAGE,
            response.GetResult().GetErrorResponse().message);
  EXPECT_TRUE(response.GetResult().GetAccessToken().empty());
  EXPECT_TRUE(response.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationOnlineTest, SignInClientCancel) {
  AuthenticationCredentials credentials(id_, secret_);

  std::time_t now;
  AuthenticationClient::SignInClientResponse response =
      SignInClient(credentials, now, LIMIT_EXPIRY, true);

  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(AuthenticationOnlineTest, SignUpInUser) {
  std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse signUpResponse = SignUpUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::CREATED,
            signUpResponse.GetResult().GetStatus());
  EXPECT_EQ(ERROR_SIGNUP_CREATED,
            signUpResponse.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(signUpResponse.GetResult().GetUserIdentifier().empty());

  AuthenticationClient::SignInUserResponse response = SignInUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::PRECONDITION_FAILED,
            response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_PRECONDITION_FAILED_CODE,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_PRECONDITION_FAILED_MESSAGE,
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
  EXPECT_EQ(ERROR_NO_CONTENT, response2.GetResult().GetErrorResponse().message);
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
  EXPECT_STREQ(ERROR_OK.c_str(),
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
  EXPECT_STREQ(ERROR_NO_CONTENT.c_str(), response4.error.c_str());

  AuthenticationClient::SignInUserResponse response5 = SignInUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::UNAUTHORIZED,
            response5.GetResult().GetStatus());
  EXPECT_EQ(ERROR_ACCOUNT_NOT_FOUND_CODE,
            response5.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_ACCOUNT_NOT_FOUND_MESSAGE,
            response5.GetResult().GetErrorResponse().message);
}

TEST_F(AuthenticationOnlineTest, SignUpUserCancel) {
  std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse response =
      SignUpUser(email, "password123", true);
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(AuthenticationOnlineTest, SignInUserCancel) {
  std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse signUpResponse = SignUpUser(email);
  EXPECT_TRUE(signUpResponse.IsSuccessful());

  AuthenticationClient::SignInUserResponse response = SignInUser(email, true);
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(AuthenticationOnlineTest, AcceptTermCancel) {
  std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse signUpResponse = SignUpUser(email);
  EXPECT_TRUE(signUpResponse.IsSuccessful());

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

TEST_F(AuthenticationOnlineTest, SignInRefresh) {
  std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse signUpResponse = SignUpUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::CREATED,
            signUpResponse.GetResult().GetStatus());
  EXPECT_EQ(ERROR_SIGNUP_CREATED,
            signUpResponse.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(signUpResponse.GetResult().GetUserIdentifier().empty());

  AuthenticationClient::SignInUserResponse response = SignInUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::PRECONDITION_FAILED,
            response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_PRECONDITION_FAILED_CODE,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_PRECONDITION_FAILED_MESSAGE,
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
  EXPECT_EQ(ERROR_NO_CONTENT, response2.GetResult().GetErrorResponse().message);
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
  EXPECT_EQ(ERROR_OK, response3.GetResult().GetErrorResponse().message);
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
  EXPECT_EQ(ERROR_OK, response4.GetResult().GetErrorResponse().message);
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
  EXPECT_EQ(ERROR_REFRESH_FAILED_CODE,
            response5.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_REFRESH_FAILED_MESSAGE,
            response5.GetResult().GetErrorResponse().message);

  AuthenticationUtils::DeleteUserResponse response6 =
      DeleteUser(response4.GetResult().GetAccessToken());
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT, response6.status);
  EXPECT_STREQ(ERROR_NO_CONTENT.c_str(), response6.error.c_str());

  AuthenticationClient::SignInUserResponse response7 = SignInUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::UNAUTHORIZED,
            response7.GetResult().GetStatus());
  EXPECT_EQ(ERROR_ACCOUNT_NOT_FOUND_CODE,
            response7.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_ACCOUNT_NOT_FOUND_MESSAGE,
            response7.GetResult().GetErrorResponse().message);
}

TEST_F(AuthenticationOnlineTest, SignInRefreshCancel) {
  std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse signUpResponse = SignUpUser(email);
  EXPECT_TRUE(signUpResponse.IsSuccessful());

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

TEST_F(AuthenticationOnlineTest, SignOutUser) {
  std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignUpResponse signUpResponse = SignUpUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::CREATED,
            signUpResponse.GetResult().GetStatus());
  EXPECT_EQ(ERROR_SIGNUP_CREATED,
            signUpResponse.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(signUpResponse.GetResult().GetUserIdentifier().empty());

  AuthenticationClient::SignInUserResponse response = SignInUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::PRECONDITION_FAILED,
            response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_PRECONDITION_FAILED_CODE,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_PRECONDITION_FAILED_MESSAGE,
            response.GetResult().GetErrorResponse().message);

  AuthenticationClient::SignInUserResponse response2 = AcceptTerms(response);
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT,
            response2.GetResult().GetStatus());
  EXPECT_STREQ(ERROR_NO_CONTENT.c_str(),
               response2.GetResult().GetErrorResponse().message.c_str());

  AuthenticationClient::SignInUserResponse response3 = SignInUser(email);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response3.GetResult().GetStatus());
  EXPECT_STREQ(ERROR_OK.c_str(),
               response3.GetResult().GetErrorResponse().message.c_str());

  AuthenticationClient::SignOutUserResponse signOutResponse =
      SignOutUser(response3.GetResult().GetAccessToken());
  EXPECT_TRUE(signOutResponse.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT,
            signOutResponse.GetResult().GetStatus());
  EXPECT_EQ(ERROR_NO_CONTENT,
            signOutResponse.GetResult().GetErrorResponse().message);

  AuthenticationUtils::DeleteUserResponse response4 =
      DeleteUser(response3.GetResult().GetAccessToken());
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT, response4.status);
  EXPECT_STREQ(ERROR_NO_CONTENT.c_str(), response4.error.c_str());
}

TEST_F(AuthenticationOnlineTest, NetworkProxySettings) {
  AuthenticationCredentials credentials(id_, secret_);

  auto proxySettings = olp::http::NetworkProxySettings();
  client_->SetNetworkProxySettings(proxySettings);
  proxySettings.WithHostname("$.?");
  proxySettings.WithPort(42);
  proxySettings.WithType(olp::http::NetworkProxySettings::Type::SOCKS4);

  client_->SetNetworkProxySettings(proxySettings);
  std::time_t now;
  auto response = SignInClient(credentials, now, EXPIRY_TIME);
  // Bad proxy error code and message varies by platform
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_TRUE(response.GetError().GetErrorCode() ==
              olp::client::ErrorCode::ServiceUnavailable);
  EXPECT_STRNE(response.GetError().GetMessage().c_str(), ERROR_OK.c_str());
}

TEST_F(AuthenticationOnlineTest, ErrorFields) {
  AuthenticationClient::SignUpResponse signUpResponse =
      SignUpUser("a/*<@test.com", "password");
  EXPECT_TRUE(signUpResponse.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
            signUpResponse.GetResult().GetStatus());
  EXPECT_EQ(ERROR_FIEDS_CODE,
            signUpResponse.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_FIELDS_MESSAGE,
            signUpResponse.GetResult().GetErrorResponse().message);
  EXPECT_EQ(2, signUpResponse.GetResult().GetErrorFields().size());
  int count = 0;
  for (ErrorFields::const_iterator it =
           signUpResponse.GetResult().GetErrorFields().begin();
       it != signUpResponse.GetResult().GetErrorFields().end(); it++) {
    std::string name = count == 0 ? EMAIL : PASSWORD;
    std::string message =
        count == 0 ? ERROR_ILLEGAL_EMAIL : ERROR_BLACKLISTED_PASSWORD;
    unsigned int code =
        count == 0 ? ERROR_ILLEGAL_EMAIL_CODE : ERROR_BLACKLISTED_PASSWORD_CODE;
    count++;
    EXPECT_EQ(name, it->name);
    EXPECT_EQ(message, it->message);
    EXPECT_EQ(code, it->code);
  }
}
