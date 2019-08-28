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

#include "AuthenticationTests.h"

#include "olp/core/network/HttpStatusCode.h"

TEST_F(AuthenticationOfflineTest, SignInClientData) {
  AuthenticationCredentials credentials(id_, secret_);
  std::promise<AuthenticationClient::SignInClientResponse> request;
  auto request_future = request.get_future();

  auto expectation = ExpectationBuilder();
  expectation->forUrl(signin_request)
      .withResponseData(std::vector<char>(response_1.begin(), response_1.end()))
      .withReturnCode(olp::network::HttpStatusCode::Ok)
      .withErrorString(ERROR_OK)
      .completeSynchronously()
      .buildExpectation();

  std::time_t now = std::time(nullptr);
  client_->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInClientResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_EQ(
      "tyJhbGciOiJSUzUxMiIsImN0eSI6IkpXVCIsImlzcyI6IkhFUkUiLCJhaWQiOiJTcFR5dkQ0"
      "RjZ1dWhVY0t3Zj"
      "BPRC"
      "IsImlhdCI6MTUyMjY5OTY2MywiZXhwIjoxNTIyNzAzMjYzLCJraWQiOiJqMSJ9."
      "ZXlKaGJHY2lPaUprYVhJaUxDSmxibU1pT2lKQk1qVTJRMEpETFVoVE5URXlJbjAuLkNuSXBW"
      "VG14bFBUTFhqdF"
      "l0OD"
      "VodVEuTk1aMzRVSndtVnNOX21Zd3pwa1UydVFfMklCbE9QeWw0VEJWQnZXczcwRXdoQWRld0"
      "tpR09KOGFHOWtK"
      "eTBo"
      "YWg2SS03Y01WbXQ4S3ppUHVKOXZqV2U1Q0F4cER0LU0yQUxhQTJnZWlIZXJuaEEwZ1ZRR3pV"
      "akw5OEhDdkpEc2"
      "YuQX"
      "hxNTRPTG9FVDhqV2ZreTgtZHY4ZUR1SzctRnJOWklGSms0RHZGa2F5Yw.bfSc5sXovW0-"
      "yGTqWDZtsVvqIxeNl9IGFbtzRBRkHCHEjthZzeRscB6oc707JTpiuRmDKJe6oFU03RocTS99"
      "YBlM3p5rP2moad"
      "DNmP"
      "3Uag4elo6z0ZE_w1BP7So7rMX1k4NymfEATdmyXVnjAhBlTPQqOYIWV-"
      "UNCXWCIzLSuwaJ96N1d8XZeiA1jkpsp4CKfcSSm9hgsKNA95SWPnZAHyqOYlO0sDE28osOIj"
      "N2UVSUKlO1BDtL"
      "iPLt"
      "a_dIqvqFUU5aRi_"
      "dcYqkJcZh195ojzeAcvDGI6HqS2zUMTdpYUhlwwfpkxGwrFmlAxgx58xKSeVt0sPvtabZBAW"
      "8uh2NGg",
      response.GetResult().GetAccessToken());
  EXPECT_GE(now + MAX_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());

  // Check if we can get token when offline
  auto expectation2 = ExpectationBuilder();
  expectation2->forUrl(signin_request)
      .withResponseData(std::vector<char>())
      .withReturnCode(-1)
      .withErrorString("")
      .completeSynchronously()
      .buildExpectation();

  std::promise<AuthenticationClient::SignInClientResponse> request_2;
  auto request_future_2 = request_2.get_future();
  client_->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request_2.set_value(response);
      });
  request_future_2.wait();

  AuthenticationClient::SignInClientResponse response_2 =
      request_future_2.get();
  EXPECT_TRUE(response_2.IsSuccessful());
  EXPECT_EQ(
      "tyJhbGciOiJSUzUxMiIsImN0eSI6IkpXVCIsImlzcyI6IkhFUkUiLCJhaWQiOiJTcFR5dkQ0"
      "RjZ1dWhVY0t3Zj"
      "BPRC"
      "IsImlhdCI6MTUyMjY5OTY2MywiZXhwIjoxNTIyNzAzMjYzLCJraWQiOiJqMSJ9."
      "ZXlKaGJHY2lPaUprYVhJaUxDSmxibU1pT2lKQk1qVTJRMEpETFVoVE5URXlJbjAuLkNuSXBW"
      "VG14bFBUTFhqdF"
      "l0OD"
      "VodVEuTk1aMzRVSndtVnNOX21Zd3pwa1UydVFfMklCbE9QeWw0VEJWQnZXczcwRXdoQWRld0"
      "tpR09KOGFHOWtK"
      "eTBo"
      "YWg2SS03Y01WbXQ4S3ppUHVKOXZqV2U1Q0F4cER0LU0yQUxhQTJnZWlIZXJuaEEwZ1ZRR3pV"
      "akw5OEhDdkpEc2"
      "YuQX"
      "hxNTRPTG9FVDhqV2ZreTgtZHY4ZUR1SzctRnJOWklGSms0RHZGa2F5Yw.bfSc5sXovW0-"
      "yGTqWDZtsVvqIxeNl9IGFbtzRBRkHCHEjthZzeRscB6oc707JTpiuRmDKJe6oFU03RocTS99"
      "YBlM3p5rP2moad"
      "DNmP"
      "3Uag4elo6z0ZE_w1BP7So7rMX1k4NymfEATdmyXVnjAhBlTPQqOYIWV-"
      "UNCXWCIzLSuwaJ96N1d8XZeiA1jkpsp4CKfcSSm9hgsKNA95SWPnZAHyqOYlO0sDE28osOIj"
      "N2UVSUKlO1BDtL"
      "iPLt"
      "a_dIqvqFUU5aRi_"
      "dcYqkJcZh195ojzeAcvDGI6HqS2zUMTdpYUhlwwfpkxGwrFmlAxgx58xKSeVt0sPvtabZBAW"
      "8uh2NGg",
      response.GetResult().GetAccessToken());
  ;
  EXPECT_GE(now + MAX_EXPIRY, response_2.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_EXPIRY, response_2.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response_2.GetResult().GetTokenType());
  EXPECT_TRUE(response_2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_2.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationOfflineTest, SignUpHereUserData) {
  auto expectation = ExpectationBuilder();
  expectation->forUrl(signup_request)
      .withResponseData(std::vector<char>(signup_here_user_response.begin(),
                                          signup_here_user_response.end()))
      .withReturnCode(olp::network::HttpStatusCode::Created)
      .withErrorString(ERROR_SIGNUP_CREATED)
      .completeSynchronously()
      .buildExpectation();

  AuthenticationClient::SignUpResponse signUpResponse =
      SignUpUser("email@example.com");
  EXPECT_TRUE(signUpResponse.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::Created,
            signUpResponse.GetResult().GetStatus());
  EXPECT_EQ(ERROR_SIGNUP_CREATED,
            signUpResponse.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(signUpResponse.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationOfflineTest, SignInUserDataFirstTime) {
  AuthenticationCredentials credentials(id_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  auto expectation = ExpectationBuilder();
  expectation->forUrl(signin_request)
      .withResponseData(
          std::vector<char>(user_signinuser_first_time_response.begin(),
                            user_signinuser_first_time_response.end()))
      .withReturnCode(olp::network::HttpStatusCode::PreconditionFailed)
      .withErrorString(ERROR_PRECONDITION_FAILED_MESSAGE)
      .completeSynchronously()
      .buildExpectation();

  AuthenticationClient::UserProperties properties;
  client_->SignInHereUser(
      credentials, properties,
      [&](const AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInUserResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::PreconditionFailed,
            response.GetResult().GetStatus());
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
}

TEST_F(AuthenticationOfflineTest, AcceptTermsData) {
  AuthenticationCredentials credentials(id_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  auto expectation = ExpectationBuilder();
  expectation->forUrl(accept_request)
      .withResponseData(std::vector<char>(response_no_content.begin(),
                                          response_no_content.end()))
      .withReturnCode(olp::network::HttpStatusCode::NoContent)
      .withErrorString(ERROR_NO_CONTENT)
      .completeSynchronously()
      .buildExpectation();

  client_->AcceptTerms(
      credentials, "reacceptance_token",
      [&](const AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInUserResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent,
            response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_NO_CONTENT, response.GetResult().GetErrorResponse().message);
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

TEST_F(AuthenticationOfflineTest, SignInHereUser) {
  AuthenticationCredentials credentials(id_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  auto expectation = ExpectationBuilder();
  expectation->forUrl(signin_request)
      .withResponseData(std::vector<char>(user_signin_response.begin(),
                                          user_signin_response.end()))
      .withReturnCode(olp::network::HttpStatusCode::Ok)
      .withErrorString(ERROR_OK)
      .completeSynchronously()
      .buildExpectation();

  AuthenticationClient::UserProperties properties;
  std::time_t now = std::time(nullptr);
  client_->SignInHereUser(
      credentials, properties,
      [&](const AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInUserResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::Ok, response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_OK, response.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_EQ("password_grant_token", response.GetResult().GetAccessToken());
  EXPECT_GE(now + MAX_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_FALSE(response.GetResult().GetRefreshToken().empty());
  EXPECT_FALSE(response.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationOfflineTest, SignOutUser) {
  AuthenticationCredentials credentials(id_, secret_);
  std::promise<AuthenticationClient::SignOutUserResponse> request;
  auto request_future = request.get_future();

  auto expectation = ExpectationBuilder();
  expectation->forUrl(signout_request)
      .withResponseData(std::vector<char>(response_no_content.begin(),
                                          response_no_content.end()))
      .withReturnCode(olp::network::HttpStatusCode::NoContent)
      .withErrorString(ERROR_NO_CONTENT)
      .completeSynchronously()
      .buildExpectation();

  client_->SignOut(
      credentials,
      "h1.C33vsPr8atTZcXOC7AWbgQ.hCGWE5CNLuQv4vSLJUOAqGuRNjhO34qCH8mZIQ-"
      "93gBqlf34y37DNl92FUnPrgECxojv7rn4bXYRZDohlx1o91bMgQH20G2N94bdrl2pOB9XT_"
      "rqT54anW_XfGZAZQRwPz8RRayuNBcf_FGDFyn0YFP0_"
      "c4tH8yg6HI0YK5sL7N09JRhoBKdhseBH_"
      "QOiR2IYJsvlhcjkdduHHWFQQpDNPwlqRLJ9ivfwSVmsxIgxra2x85bxdkg1kY-"
      "H5ZeBIFkyxs6E__"
      "OT5aYPWhOoR2aqdtVUWtcQDuqccEKzXDcs8dYVKgU7jxyUG6GspW67397EK-"
      "XOPbk9IqTPNpOFOioVeZn1ylw5FuriUpsxAUX8VS7DOldw5mQ-"
      "OyE91MDGvItecI6PmRDSdyE5c9xTQ759vY07sUStP0K-Cq65UKqwysN_"
      "3qSvgqcFotalyUMbtYoW0DGquS7aORdK0azI2LT2Q.bpjVuX3Zr69to7dHhZoTXGulAmLv_"
      "ES4Ne1d3bQ7XiWVggDeRGzZvg-49P0cTz146aV7ugl71-"
      "opH2ATwLFekphRd8NaYcc2aVKo4stZgBr6ZVvO9HKqvZZ02lPbQXJuRqt1yEmEpLIMJbD-"
      "o8M8_"
      "Im2mE_NbivYDZkYSzz-"
      "pIw5c0qHluBFF3e8QSVU99dNOBLrHTQ51j3qejLQ3q8DQzKYfg3EMMstVH6VC4xvWabn0a3-"
      "TQHbrQ-P_h4Ei5oP10Kmhur-lGmMBomAaByHWulqTyv19RXvAIC4rg_b2OYA-"
      "uzPwcDGeDB5h24l08Cgxq7r7mPKcwSgTOHZY4oaaA",
      [&](const AuthenticationClient::SignOutUserResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignOutUserResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  SignOutResult s = response.GetResult();
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent, s.GetStatus());
  EXPECT_EQ(ERROR_NO_CONTENT, response.GetResult().GetErrorResponse().message);
}

TEST_F(AuthenticationOfflineTest, SignInFacebookData) {
  AuthenticationCredentials credentials(id_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  auto expectation = ExpectationBuilder();
  expectation->forUrl(signin_request)
      .withResponseData(std::vector<char>(facebook_signin_response.begin(),
                                          facebook_signin_response.end()))
      .withReturnCode(olp::network::HttpStatusCode::Ok)
      .withErrorString(ERROR_OK)
      .completeSynchronously()
      .buildExpectation();

  AuthenticationClient::FederatedProperties properties;
  std::time_t now = std::time(nullptr);
  client_->SignInFacebook(
      credentials, properties,
      [&](const AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInUserResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::Ok, response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_OK, response.GetResult().GetErrorResponse().message);
  EXPECT_EQ("facebook_grant_token", response.GetResult().GetAccessToken());
  EXPECT_GE(now + MAX_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_EQ("5j687leur4njgb4osomifn55p0",
            response.GetResult().GetRefreshToken());
  EXPECT_FALSE(response.GetResult().GetUserIdentifier().empty());
  EXPECT_EQ("HERE-5fa10eda-39ff-4cbc-9b0c-5acba4685649",
            response.GetResult().GetUserIdentifier());
  EXPECT_TRUE(response.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_TRUE(response.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_TRUE(response.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_TRUE(response.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_TRUE(response.GetResult().GetPrivatePolicyUrlJson().empty());
}

TEST_F(AuthenticationOfflineTest, SignInGoogleData) {
  AuthenticationCredentials credentials(id_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  auto expectation = ExpectationBuilder();
  expectation->forUrl(signin_request)
      .withResponseData(std::vector<char>(google_signin_response.begin(),
                                          google_signin_response.end()))
      .withReturnCode(olp::network::HttpStatusCode::Ok)
      .withErrorString(ERROR_OK)
      .completeSynchronously()
      .buildExpectation();

  AuthenticationClient::FederatedProperties properties;
  std::time_t now = std::time(nullptr);
  client_->SignInGoogle(
      credentials, properties,
      [&](const AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInUserResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::Ok, response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_OK, response.GetResult().GetErrorResponse().message);
  EXPECT_EQ("google_grant_token", response.GetResult().GetAccessToken());
  EXPECT_GE(now + MAX_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_FALSE(response.GetResult().GetRefreshToken().empty());
  EXPECT_FALSE(response.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationOfflineTest, SignInArcGisData) {
  AuthenticationCredentials credentials(id_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  auto expectation = ExpectationBuilder();
  expectation->forUrl(signin_request)
      .withResponseData(std::vector<char>(arcgis_signin_response.begin(),
                                          arcgis_signin_response.end()))
      .withReturnCode(olp::network::HttpStatusCode::Ok)
      .withErrorString(ERROR_OK)
      .completeSynchronously()
      .buildExpectation();

  std::time_t now = std::time(nullptr);
  AuthenticationClient::FederatedProperties properties;
  client_->SignInArcGis(
      credentials, properties,
      [&](const AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInUserResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::Ok, response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_OK, response.GetResult().GetErrorResponse().message);
  EXPECT_EQ("arcgis_grant_token", response.GetResult().GetAccessToken());
  EXPECT_GE(now + MAX_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_EQ("5j687leur4njgb4osomifn55p0",
            response.GetResult().GetRefreshToken());
  EXPECT_FALSE(response.GetResult().GetUserIdentifier().empty());
  EXPECT_EQ("HERE-5fa10eda-39ff-4cbc-9b0c-5acba4685649",
            response.GetResult().GetUserIdentifier());
  EXPECT_TRUE(response.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_TRUE(response.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_TRUE(response.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_TRUE(response.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_TRUE(response.GetResult().GetPrivatePolicyUrlJson().empty());
}

TEST_F(AuthenticationOfflineTest, SignInRefreshData) {
  AuthenticationCredentials credentials(id_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  auto expectation = ExpectationBuilder();
  expectation->forUrl(signin_request)
      .withResponseData(std::vector<char>(refresh_signin_response.begin(),
                                          refresh_signin_response.end()))
      .withReturnCode(olp::network::HttpStatusCode::Ok)
      .withErrorString(ERROR_OK)
      .completeSynchronously()
      .buildExpectation();

  AuthenticationClient::UserProperties properties;
  std::time_t now = std::time(nullptr);
  client_->SignInHereUser(
      credentials, properties,
      [&](const AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInUserResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::Ok, response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_OK, response.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_EQ("refresh_grant_token", response.GetResult().GetAccessToken());
  EXPECT_GE(now + MAX_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_FALSE(response.GetResult().GetRefreshToken().empty());
  EXPECT_FALSE(response.GetResult().GetUserIdentifier().empty());
}

void checkErrorFields(const ErrorFields& errorFields) {
  EXPECT_EQ(2, errorFields.size());
  int count = 0;
  for (ErrorFields::const_iterator it = errorFields.begin();
       it != errorFields.end(); it++) {
    std::string name = count == 0 ? PASSWORD : LAST_NAME;
    std::string message =
        count == 0 ? ERROR_BLACKLISTED_PASSWORD : ERROR_ILLEGAL_LAST_NAME;
    unsigned int code = count == 0 ? ERROR_BLACKLISTED_PASSWORD_CODE
                                   : ERROR_ILLEGAL_LAST_NAME_CODE;
    count++;
    EXPECT_EQ(name, it->name);
    EXPECT_EQ(message, it->message);
    EXPECT_EQ(code, it->code);
  }
}

TEST_F(AuthenticationOfflineTest, ErrorFieldsData) {
  AuthenticationCredentials credentials(id_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  auto expectation = ExpectationBuilder();
  expectation->forUrl(signin_request)
      .withResponseData(std::vector<char>(response_error_fields.begin(),
                                          response_error_fields.end()))
      .withReturnCode(olp::network::HttpStatusCode::BadRequest)
      .withErrorString(ERROR_FIELDS_MESSAGE)
      .completeSynchronously()
      .buildExpectation();

  AuthenticationClient::UserProperties properties;
  client_->SignInHereUser(
      credentials, properties,
      [&](const AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInUserResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::BadRequest,
            response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_FIEDS_CODE, response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_FIELDS_MESSAGE,
            response.GetResult().GetErrorResponse().message);
  checkErrorFields(response.GetResult().GetErrorFields());

  expectation->forUrl(signout_request)
      .withResponseData(std::vector<char>(response_error_fields.begin(),
                                          response_error_fields.end()))
      .withReturnCode(olp::network::HttpStatusCode::BadRequest)
      .withErrorString(ERROR_FIELDS_MESSAGE)
      .completeSynchronously()
      .buildExpectation();

  AuthenticationClient::SignOutUserResponse signOutResponse =
      SignOutUser("token");
  EXPECT_TRUE(signOutResponse.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::BadRequest,
            signOutResponse.GetResult().GetStatus());
  EXPECT_EQ(ERROR_FIEDS_CODE,
            signOutResponse.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_FIELDS_MESSAGE,
            signOutResponse.GetResult().GetErrorResponse().message);
  checkErrorFields(response.GetResult().GetErrorFields());

  expectation->forUrl(signup_request)
      .withResponseData(std::vector<char>(response_error_fields.begin(),
                                          response_error_fields.end()))
      .withReturnCode(olp::network::HttpStatusCode::BadRequest)
      .withErrorString(ERROR_FIELDS_MESSAGE)
      .completeSynchronously()
      .buildExpectation();
  AuthenticationClient::SignUpResponse signup_response = SignUpUser("email");
  EXPECT_TRUE(signup_response.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::BadRequest,
            signup_response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_FIEDS_CODE,
            signup_response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_FIELDS_MESSAGE,
            signup_response.GetResult().GetErrorResponse().message);
  checkErrorFields(response.GetResult().GetErrorFields());
}

TEST_F(AuthenticationOfflineTest, TestInvalidResponses) {
  ExecuteSigninRequest(olp::network::HttpStatusCode::Ok,
                       olp::network::HttpStatusCode::ServiceUnavailable,
                       ERROR_SERVICE_UNAVAILABLE, response_invalid_json);
  ExecuteSigninRequest(olp::network::HttpStatusCode::Ok,
                       olp::network::HttpStatusCode::ServiceUnavailable,
                       ERROR_SERVICE_UNAVAILABLE, response_no_token);
  ExecuteSigninRequest(olp::network::HttpStatusCode::Ok,
                       olp::network::HttpStatusCode::ServiceUnavailable,
                       ERROR_SERVICE_UNAVAILABLE, response_no_token_type);
  ExecuteSigninRequest(olp::network::HttpStatusCode::Ok,
                       olp::network::HttpStatusCode::ServiceUnavailable,
                       ERROR_SERVICE_UNAVAILABLE, response_no_expiry);
}

TEST_F(AuthenticationOfflineTest, TestHttpRequestErrorCodes) {
  ExecuteSigninRequest(olp::network::HttpStatusCode::Accepted,
                       olp::network::HttpStatusCode::Accepted, ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::Created,
                       olp::network::HttpStatusCode::Created, ERROR_UNDEFINED,
                       response_created);
  ExecuteSigninRequest(
      olp::network::HttpStatusCode::NonAuthoritativeInformation,
      olp::network::HttpStatusCode::NonAuthoritativeInformation,
      ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::NoContent,
                       olp::network::HttpStatusCode::NoContent, ERROR_UNDEFINED,
                       response_no_content);
  ExecuteSigninRequest(olp::network::HttpStatusCode::ResetContent,
                       olp::network::HttpStatusCode::ResetContent,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::PartialContent,
                       olp::network::HttpStatusCode::PartialContent,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::MultipleChoices,
                       olp::network::HttpStatusCode::MultipleChoices,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::MovedPermanently,
                       olp::network::HttpStatusCode::MovedPermanently,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::Found,
                       olp::network::HttpStatusCode::Found, ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::SeeOther,
                       olp::network::HttpStatusCode::SeeOther, ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::NotModified,
                       olp::network::HttpStatusCode::NotModified,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::UseProxy,
                       olp::network::HttpStatusCode::UseProxy, ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::BadRequest,
                       olp::network::HttpStatusCode::BadRequest,
                       ERROR_BAD_REQUEST_MESSAGE, response_bad_request,
                       ERROR_BAD_REQUEST_CODE);
  ExecuteSigninRequest(olp::network::HttpStatusCode::Unauthorized,
                       olp::network::HttpStatusCode::Unauthorized,
                       ERROR_UNAUTHORIZED_MESSAGE, response_unauthorized,
                       ERROR_UNAUTHORIZED_CODE);
  ExecuteSigninRequest(olp::network::HttpStatusCode::PaymentRequired,
                       olp::network::HttpStatusCode::PaymentRequired,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::NotFound,
                       olp::network::HttpStatusCode::NotFound,
                       ERROR_USER_NOT_FOUND, response_not_found,
                       ERROR_NOT_FOUND_CODE);
  ExecuteSigninRequest(olp::network::HttpStatusCode::MethodNotAllowed,
                       olp::network::HttpStatusCode::MethodNotAllowed,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::Forbidden,
                       olp::network::HttpStatusCode::Forbidden,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::NotAcceptable,
                       olp::network::HttpStatusCode::NotAcceptable,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(
      olp::network::HttpStatusCode::ProxyAuthenticationRequired,
      olp::network::HttpStatusCode::ProxyAuthenticationRequired,
      ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::RequestTimeout,
                       olp::network::HttpStatusCode::RequestTimeout,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::Conflict,
                       olp::network::HttpStatusCode::Conflict,
                       ERROR_CONFLICT_MESSAGE, response_conflict,
                       ERROR_CONFLICT_CODE);
  ExecuteSigninRequest(olp::network::HttpStatusCode::Gone,
                       olp::network::HttpStatusCode::Gone, ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::LengthRequired,
                       olp::network::HttpStatusCode::LengthRequired,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::PreconditionFailed,
                       olp::network::HttpStatusCode::PreconditionFailed,
                       ERROR_PRECONDITION_FAILED_MESSAGE,
                       response_precodition_failed);
  ExecuteSigninRequest(olp::network::HttpStatusCode::RequestEntityTooLarge,
                       olp::network::HttpStatusCode::RequestEntityTooLarge,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::RequestUriTooLong,
                       olp::network::HttpStatusCode::RequestUriTooLong,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::UnsupportedMediaType,
                       olp::network::HttpStatusCode::UnsupportedMediaType,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::TooManyRequests,
                       olp::network::HttpStatusCode::TooManyRequests,
                       ERROR_TOO_MANY_REQUESTS_MESSAGE,
                       response_too_many_requests,
                       ERROR_TOO_MANY_REQUESTS_CODE);
  ExecuteSigninRequest(olp::network::HttpStatusCode::InternalServerError,
                       olp::network::HttpStatusCode::InternalServerError,
                       ERROR_INTERNAL_SERVER_MESSAGE,
                       response_internal_server_error,
                       ERROR_INTERNAL_SERVER_CODE);
  ExecuteSigninRequest(olp::network::HttpStatusCode::NotImplemented,
                       olp::network::HttpStatusCode::NotImplemented,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::BadGateway,
                       olp::network::HttpStatusCode::BadGateway,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::ServiceUnavailable,
                       olp::network::HttpStatusCode::ServiceUnavailable,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::GatewayTimeout,
                       olp::network::HttpStatusCode::GatewayTimeout,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(olp::network::HttpStatusCode::VersionNotSupported,
                       olp::network::HttpStatusCode::VersionNotSupported,
                       ERROR_UNDEFINED);
  ExecuteSigninRequest(100000, 100000, ERROR_UNDEFINED);
  ExecuteSigninRequest(-100000, -100000, ERROR_UNDEFINED);
}

TEST_F(AuthenticationOnlineTest, SignInClient) {
  AuthenticationCredentials credentials(id_, secret_);
  std::time_t now;
  AuthenticationClient::SignInClientResponse response =
      SignInClient(credentials, now, EXPIRY_TIME);
  EXPECT_EQ(olp::network::HttpStatusCode::Ok, response.GetResult().GetStatus());
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
  EXPECT_EQ(olp::network::HttpStatusCode::Ok,
            response_2.GetResult().GetStatus());
  EXPECT_FALSE(response_2.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + MAX_EXTENDED_EXPIRY, response_2.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_EXTENDED_EXPIRY, response_2.GetResult().GetExpiryTime());
  EXPECT_FALSE(response_2.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response_2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_2.GetResult().GetUserIdentifier().empty());

  now = std::time(nullptr);
  AuthenticationClient::SignInClientResponse response_3 =
      SignInClient(credentials, now, CUSTOM_EXPIRY_TIME);
  EXPECT_EQ(olp::network::HttpStatusCode::Ok,
            response_3.GetResult().GetStatus());
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
  EXPECT_EQ(olp::network::HttpStatusCode::Ok, response.GetResult().GetStatus());
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_STREQ(ERROR_OK.c_str(),
               response.GetResult().GetErrorResponse().message.c_str());
  EXPECT_GE(now + MAX_LIMIT_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_LIMIT_EXPIRY, response.GetResult().GetExpiryTime());

  // Test token expiration greater than 24h
  AuthenticationClient::SignInClientResponse response_2 =
      SignInClient(credentials, now, 90000);
  EXPECT_EQ(olp::network::HttpStatusCode::Ok,
            response_2.GetResult().GetStatus());
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
  EXPECT_EQ(olp::network::HttpStatusCode::Unauthorized,
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
  EXPECT_EQ(olp::network::HttpStatusCode::Created,
            signUpResponse.GetResult().GetStatus());
  EXPECT_EQ(ERROR_SIGNUP_CREATED,
            signUpResponse.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(signUpResponse.GetResult().GetUserIdentifier().empty());

  AuthenticationClient::SignInUserResponse response = SignInUser(email);
  EXPECT_EQ(olp::network::HttpStatusCode::PreconditionFailed,
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
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent,
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
  EXPECT_EQ(olp::network::HttpStatusCode::Ok,
            response3.GetResult().GetStatus());
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
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent, response4.status);
  EXPECT_STREQ(ERROR_NO_CONTENT.c_str(), response4.error.c_str());

  AuthenticationClient::SignInUserResponse response5 = SignInUser(email);
  EXPECT_EQ(olp::network::HttpStatusCode::Unauthorized,
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
  EXPECT_EQ(olp::network::HttpStatusCode::Created,
            signUpResponse.GetResult().GetStatus());
  EXPECT_EQ(ERROR_SIGNUP_CREATED,
            signUpResponse.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(signUpResponse.GetResult().GetUserIdentifier().empty());

  AuthenticationClient::SignInUserResponse response = SignInUser(email);
  EXPECT_EQ(olp::network::HttpStatusCode::PreconditionFailed,
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
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent,
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
  EXPECT_EQ(olp::network::HttpStatusCode::Ok,
            response3.GetResult().GetStatus());
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
  EXPECT_EQ(olp::network::HttpStatusCode::Ok,
            response4.GetResult().GetStatus());
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
  EXPECT_EQ(olp::network::HttpStatusCode::Unauthorized,
            response5.GetResult().GetStatus());
  EXPECT_EQ(ERROR_REFRESH_FAILED_CODE,
            response5.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_REFRESH_FAILED_MESSAGE,
            response5.GetResult().GetErrorResponse().message);

  AuthenticationUtils::DeleteUserResponse response6 =
      DeleteUser(response4.GetResult().GetAccessToken());
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent, response6.status);
  EXPECT_STREQ(ERROR_NO_CONTENT.c_str(), response6.error.c_str());

  AuthenticationClient::SignInUserResponse response7 = SignInUser(email);
  EXPECT_EQ(olp::network::HttpStatusCode::Unauthorized,
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
  EXPECT_EQ(olp::network::HttpStatusCode::PreconditionFailed,
            response.GetResult().GetStatus());

  AuthenticationClient::SignInUserResponse response2 = AcceptTerms(response);
  EXPECT_TRUE(response2.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent,
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
  EXPECT_EQ(olp::network::HttpStatusCode::Created,
            signUpResponse.GetResult().GetStatus());
  EXPECT_EQ(ERROR_SIGNUP_CREATED,
            signUpResponse.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(signUpResponse.GetResult().GetUserIdentifier().empty());

  AuthenticationClient::SignInUserResponse response = SignInUser(email);
  EXPECT_EQ(olp::network::HttpStatusCode::PreconditionFailed,
            response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_PRECONDITION_FAILED_CODE,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_PRECONDITION_FAILED_MESSAGE,
            response.GetResult().GetErrorResponse().message);

  AuthenticationClient::SignInUserResponse response2 = AcceptTerms(response);
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent,
            response2.GetResult().GetStatus());
  EXPECT_STREQ(ERROR_NO_CONTENT.c_str(),
               response2.GetResult().GetErrorResponse().message.c_str());

  AuthenticationClient::SignInUserResponse response3 = SignInUser(email);
  EXPECT_EQ(olp::network::HttpStatusCode::Ok,
            response3.GetResult().GetStatus());
  EXPECT_STREQ(ERROR_OK.c_str(),
               response3.GetResult().GetErrorResponse().message.c_str());

  AuthenticationClient::SignOutUserResponse signOutResponse =
      SignOutUser(response3.GetResult().GetAccessToken());
  EXPECT_TRUE(signOutResponse.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent,
            signOutResponse.GetResult().GetStatus());
  EXPECT_EQ(ERROR_NO_CONTENT,
            signOutResponse.GetResult().GetErrorResponse().message);

  AuthenticationUtils::DeleteUserResponse response4 =
      DeleteUser(response3.GetResult().GetAccessToken());
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent, response4.status);
  EXPECT_STREQ(ERROR_NO_CONTENT.c_str(), response4.error.c_str());
}

TEST_F(AuthenticationOnlineTest, NetworkProxySettings) {
  AuthenticationCredentials credentials(id_, secret_);

  auto proxySettings = NetworkProxySettings();
  EXPECT_FALSE(client_->SetNetworkProxySettings(proxySettings));

  proxySettings.host = "foo.bar";
  proxySettings.port = 42;
  EXPECT_TRUE(client_->SetNetworkProxySettings(proxySettings));
  std::time_t now;
  auto response = SignInClient(credentials, now, EXPIRY_TIME);
  // Bad proxy error code and message varies by platform
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_TRUE(response.GetError().GetErrorCode() ==
                  olp::client::ErrorCode::NetworkConnection ||
              response.GetError().GetErrorCode() ==
                  olp::client::ErrorCode::NotFound);
  EXPECT_STRNE(response.GetError().GetMessage().c_str(), ERROR_OK.c_str());
}

TEST_F(FacebookAuthenticationOnlineTest, SignInFacebook) {
  AuthenticationClient::SignInUserResponse response = SignInFacebook();
  EXPECT_EQ(olp::network::HttpStatusCode::Created,
            response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_PRECONDITION_CREATED_CODE,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_PRECONDITION_CREATED_MESSAGE,
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
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent,
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

  AuthenticationClient::SignInUserResponse response3 = SignInFacebook();
  EXPECT_EQ(olp::network::HttpStatusCode::Ok,
            response3.GetResult().GetStatus());
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

  AuthenticationUtils::DeleteUserResponse response4 =
      DeleteUser(response3.GetResult().GetAccessToken());
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent, response4.status);
  EXPECT_STREQ(ERROR_NO_CONTENT.c_str(), response4.error.c_str());

  // SignIn with invalid token
  AuthenticationClient::SignInUserResponse response5 = SignInFacebook("12345");
  EXPECT_EQ(olp::network::HttpStatusCode::Unauthorized,
            response5.GetResult().GetStatus());
  EXPECT_EQ(ERROR_FB_FAILED_CODE,
            response5.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_FB_FAILED_MESSAGE,
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

TEST_F(GoogleAuthenticationOnlineTest, SignInGoogle) {
  std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  ASSERT_FALSE(testUser.access_token.empty());

  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignInUserResponse response =
      SignInGoogleUser(email, testUser.access_token);
  EXPECT_EQ(olp::network::HttpStatusCode::Created,
            response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_PRECONDITION_CREATED_CODE,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_PRECONDITION_CREATED_MESSAGE,
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

  std::cout << "termAcceptanceToken="
            << response.GetResult().GetTermAcceptanceToken() << std::endl;

  AuthenticationClient::SignInUserResponse response2 = AcceptTerms(response);
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent,
            response2.GetResult().GetStatus());
  EXPECT_STREQ(ERROR_NO_CONTENT.c_str(),
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
      SignInGoogleUser(email, testUser.access_token);
  EXPECT_EQ(olp::network::HttpStatusCode::Ok,
            response3.GetResult().GetStatus());
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

  AuthenticationClient::SignOutUserResponse signOutResponse =
      SignOutUser(response3.GetResult().GetAccessToken());
  EXPECT_TRUE(signOutResponse.IsSuccessful());
  // EXPECT_EQ(olp::network::HttpStatusCode::NoContent,
  // signOutResponse.GetResult().GetStatus());
  // EXPECT_EQ(ERROR_NO_CONTENT.c_str(),
  //          signOutResponse.GetResult().GetErrorResponse().message);

  AuthenticationUtils::DeleteUserResponse response4 =
      DeleteUser(response3.GetResult().GetAccessToken());
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent, response4.status);
  EXPECT_STREQ(ERROR_NO_CONTENT.c_str(), response4.error.c_str());

  // SignIn with invalid token
  AuthenticationClient::SignInUserResponse response5 =
      SignInGoogleUser(email, "12345");
  EXPECT_EQ(olp::network::HttpStatusCode::Unauthorized,
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

// The ArcGIS refresh token will eventually expire. This requires a manual
// update of ARCGIS_TEST_accessToken in ArcGisTestUtils.cpp
TEST_F(ArcGisAuthenticationOnlineTest, SignInArcGis) {
  std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignInUserResponse response = SignInArcGis(email);
  EXPECT_EQ(olp::network::HttpStatusCode::Created,
            response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_PRECONDITION_CREATED_CODE,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_PRECONDITION_CREATED_MESSAGE,
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
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent,
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

  AuthenticationClient::AuthenticationClient::SignInUserResponse response3 =
      SignInArcGis(email);
  EXPECT_EQ(olp::network::HttpStatusCode::Ok,
            response3.GetResult().GetStatus());
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

  AuthenticationUtils::DeleteUserResponse response4 =
      DeleteUser(response3.GetResult().GetAccessToken());
  EXPECT_EQ(olp::network::HttpStatusCode::NoContent, response4.status);
  EXPECT_STREQ(ERROR_NO_CONTENT.c_str(), response4.error.c_str());

  // SignIn with invalid token
  AuthenticationClient::SignInUserResponse response5 =
      SignInArcGis(email, "12345");
  EXPECT_EQ(olp::network::HttpStatusCode::Unauthorized,
            response5.GetResult().GetStatus());
  EXPECT_EQ(ERROR_ARCGIS_FAILED_CODE,
            response5.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_ARCGIS_FAILED_MESSAGE,
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

TEST_F(AuthenticationOnlineTest, ErrorFields) {
  AuthenticationClient::SignUpResponse signUpResponse =
      SignUpUser("a/*<@test.com", "password");
  EXPECT_TRUE(signUpResponse.IsSuccessful());
  EXPECT_EQ(olp::network::HttpStatusCode::BadRequest,
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
