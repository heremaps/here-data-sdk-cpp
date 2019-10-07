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

#include <future>
#include <memory>

#include <gmock/gmock.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/http/HttpStatusCode.h>
#include "olp/core/client/OlpClientSettingsFactory.h"
#include <olp/authentication/AuthenticationClient.h>
#include <mocks/NetworkMock.h>
#include "AuthenticationMockedResponses.h"

using namespace olp::authentication;
using testing::_;

namespace {
constexpr unsigned int kExpirtyTime = 3600;
constexpr unsigned int kMaxExpiryTime = kExpirtyTime + 30;
constexpr unsigned int kMinExpiryTime = kExpirtyTime - 10;

// HTTP errors
const std::string kErrorOk = "OK";
const std::string kErrorSignupCreated = "Created";
const std::string kErrorServiceUnavailable = "Service unavailable";

const std::string kErrorNoContent = "No Content";
const std::string kErrorFieldsMessage = "Received invalid data.";
const std::string kErrorPreconditionFailedMessage = "Precondition Failed";

const std::string kErrorUndefined = "";

const std::string kErrorBadRequestMessage = "Invalid JSON.";

const std::string kErrorUnathorizedMessage =
    "Signature mismatch. Authorization signature or client credential is "
    "wrong.";

const std::string kErrorUserNotFound =
    "User for the given access token cannot be found.";

const std::string kErrorConfliceMessage =
    "A password account with the specified email address already exists.";

const std::string kErrorTooManyRequestsMessage =
    "Request blocked because too many requests were made. Please wait for a "
    "while before making a new request.";

const std::string kErrorInternvalServerMessage =
    "Missing Thing Encrypted Secret.";

const std::string kErrorIllegalLastName = "Illegal last name.";
const std::string kErrorBlacklistedPassword = "Black listed password.";

constexpr auto kErrorFieldsCode = 400200;
constexpr auto kErrorBadRequestCode = 400002;
constexpr auto kErrorIllegalLastNameCode = 400203;
constexpr auto kErrorBlacklistedPasswordCode = 400205;
constexpr auto kErrorTooManyRequestsCode = 429002;
constexpr auto kErrorUnauthorizedCode = 401300;
constexpr auto kErrorNotFoundCode = 404000;
constexpr auto kErrorConfliceCode = 409100;
constexpr auto kErrorInternalServerCode = 500203;

void TestCheckErrorFields(const ErrorFields& errorFields) {
  static const std::string kPassword = "password";
  static const std::string kLastName = "lastname";
  static const std::string kEmail = "email";

  EXPECT_EQ(2, errorFields.size());
  int count = 0;
  for (ErrorFields::const_iterator it = errorFields.begin();
       it != errorFields.end(); it++) {
    const std::string name = count == 0 ? kPassword : kLastName;
    const std::string message =
        count == 0 ? kErrorBlacklistedPassword : kErrorIllegalLastName;
    const unsigned int code =
        count == 0 ? kErrorBlacklistedPasswordCode : kErrorIllegalLastNameCode;
    count++;
    EXPECT_EQ(name, it->name);
    EXPECT_EQ(message, it->message);
    EXPECT_EQ(code, it->code);
  }
}
}  // namespace

class AuthenticationClientTest : public ::testing::Test {
 public:
  AuthenticationClientTest() : key_("key"), secret_("secret") {}
  void SetUp() {
    client_ = std::make_unique<AuthenticationClient>(
        "https://authentication.server.url");

    network_ = std::make_shared<NetworkMock>();
    task_scheduler_ =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
    client_->SetNetwork(network_);
    client_->SetTaskScheduler(task_scheduler_);
  }

  void TearDown() {}

  AuthenticationClient::SignUpResponse SignUpUser(const std::string& email) {
    AuthenticationCredentials credentials(key_, secret_);
    std::promise<AuthenticationClient::SignUpResponse> request;
    auto request_future = request.get_future();
    olp::authentication::AuthenticationClient::SignUpProperties properties;
    properties.email = email;
    properties.password = "password123";
    properties.date_of_birth = "31/01/1980";
    properties.first_name = "AUTH_TESTER";
    properties.last_name = "HEREOS";
    properties.country_code = "USA";
    properties.language = "en";
    properties.phone_number = "+1234567890";

    auto cancel_token = client_->SignUpHereUser(
        credentials, properties,
        [&](const AuthenticationClient::SignUpResponse& response) {
          request.set_value(response);
        });

    request_future.wait();
    return request_future.get();
  }

  AuthenticationClient::SignOutUserResponse SignOutUser(
      const std::string& access_token) {
    AuthenticationCredentials credentials(key_, secret_);
    std::promise<AuthenticationClient::SignOutUserResponse> request;
    auto request_future = request.get_future();
    auto cancel_token = client_->SignOut(
        credentials, access_token,
        [&](const AuthenticationClient::SignOutUserResponse& response) {
          request.set_value(response);
        });

    request_future.wait();
    return request_future.get();
  }

  void ExecuteSigninRequest(int http, int http_result,
                            const std::string& error_message,
                            const std::string& data = "", int error_code = 0) {
    AuthenticationCredentials credentials(key_, secret_);
    std::promise<AuthenticationClient::SignInClientResponse> request;
    auto request_future = request.get_future();

    EXPECT_CALL(*network_, Send(testing::_, testing::_, testing::_, testing::_,
                                testing::_))
        .Times(1)
        .WillOnce([&](olp::http::NetworkRequest request,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback header_callback,
                      olp::http::Network::DataCallback data_callback) {
          olp::http::RequestId request_id(5);
          if (payload) {
            *payload << data;
          }
          callback(olp::http::NetworkResponse()
                       .WithRequestId(request_id)
                       .WithStatus(http));
          if (data_callback) {
            auto raw = const_cast<char*>(data.c_str());
            data_callback(reinterpret_cast<uint8_t*>(raw), 0, data.size());
          }

          return olp::http::SendOutcome(request_id);
        });

    client_->SignInClient(
        credentials,
        [&](const AuthenticationClient::SignInClientResponse& response) {
          request.set_value(response);
        });
    request_future.wait();
    AuthenticationClient::SignInClientResponse response = request_future.get();
    if (response.IsSuccessful()) {
      EXPECT_EQ(http_result, response.GetResult().GetStatus());
      EXPECT_EQ(error_message, response.GetResult().GetErrorResponse().message);
      if (error_code != 0) {
        EXPECT_EQ(error_code, response.GetResult().GetErrorResponse().code);
      }
    }
  }

 protected:
  std::shared_ptr<NetworkMock> network_;
  std::unique_ptr<olp::authentication::AuthenticationClient> client_;
  std::shared_ptr<olp::thread::TaskScheduler> task_scheduler_;
  const std::string key_;
  const std::string secret_;
};

TEST_F(AuthenticationClientTest, SignInClientData) {
  AuthenticationCredentials credentials("key_", secret_);
  std::promise<AuthenticationClient::SignInClientResponse> request;
  auto request_future = request.get_future();

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(2)
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << kResponseValidJson;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::OK));
        if (data_callback) {
          auto raw = const_cast<char*>(kResponseValidJson.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kResponseValidJson.size());
        }

        return olp::http::SendOutcome(request_id);
      })
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(6);
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(-1)
                     .WithError(""));

        return olp::http::SendOutcome(request_id);
      });

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
  EXPECT_GE(now + kMaxExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());

  std::promise<AuthenticationClient::SignInClientResponse> request_2;
  auto request_future_2 = request_2.get_future();
  std::time_t now_2 = std::time(nullptr);
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
      response_2.GetResult().GetAccessToken());
  ;
  EXPECT_GE(now_2 + kMaxExpiryTime, response_2.GetResult().GetExpiryTime());
  EXPECT_LT(now_2 + kMinExpiryTime, response_2.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response_2.GetResult().GetTokenType());
  EXPECT_TRUE(response_2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_2.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationClientTest, SignUpHereUserData) {
  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(1)
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << kSignupHereUserResponse;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::CREATED)
                     .WithError(kErrorSignupCreated));
        if (data_callback) {
          auto raw = const_cast<char*>(kSignupHereUserResponse.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kSignupHereUserResponse.size());
        }

        return olp::http::SendOutcome(request_id);
      });

  AuthenticationClient::SignUpResponse signUpResponse =
      SignUpUser("email@example.com");
  EXPECT_TRUE(signUpResponse.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::CREATED,
            signUpResponse.GetResult().GetStatus());
  EXPECT_EQ(kErrorSignupCreated,
            signUpResponse.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(signUpResponse.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationClientTest, SignInUserDataFirstTime) {
  AuthenticationCredentials credentials(key_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(1)
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << kSigninUserFirstTimeResponse;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::PRECONDITION_FAILED)
                     .WithError(kErrorPreconditionFailedMessage));
        if (data_callback) {
          auto raw = const_cast<char*>(kSigninUserFirstTimeResponse.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kSigninUserFirstTimeResponse.size());
        }

        return olp::http::SendOutcome(request_id);
      });

  AuthenticationClient::UserProperties properties;
  client_->SignInHereUser(
      credentials, properties,
      [&](const AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInUserResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::PRECONDITION_FAILED,
            response.GetResult().GetStatus());
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
}

TEST_F(AuthenticationClientTest, AcceptTermsData) {
  AuthenticationCredentials credentials(key_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(1)
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << kResponseNoContent;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::NO_CONTENT)
                     .WithError(kErrorNoContent));
        if (data_callback) {
          auto raw = const_cast<char*>(kResponseNoContent.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kResponseNoContent.size());
        }

        return olp::http::SendOutcome(request_id);
      });

  client_->AcceptTerms(
      credentials, "reacceptance_token",
      [&](const AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInUserResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT,
            response.GetResult().GetStatus());
  EXPECT_EQ(kErrorNoContent, response.GetResult().GetErrorResponse().message);
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

TEST_F(AuthenticationClientTest, SignInHereUser) {
  AuthenticationCredentials credentials(key_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(1)
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << kUserSigninResponse;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::OK)
                     .WithError(kErrorOk));
        if (data_callback) {
          auto raw = const_cast<char*>(kUserSigninResponse.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kUserSigninResponse.size());
        }

        return olp::http::SendOutcome(request_id);
      });

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
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response.GetResult().GetStatus());
  EXPECT_EQ(kErrorOk, response.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_EQ("password_grant_token", response.GetResult().GetAccessToken());
  EXPECT_GE(now + kMaxExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_FALSE(response.GetResult().GetRefreshToken().empty());
  EXPECT_FALSE(response.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationClientTest, SignOutUser) {
  AuthenticationCredentials credentials(key_, secret_);
  std::promise<AuthenticationClient::SignOutUserResponse> request;
  auto request_future = request.get_future();

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(1)
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << kResponseNoContent;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::NO_CONTENT)
                     .WithError(kErrorNoContent));
        if (data_callback) {
          auto raw = const_cast<char*>(kResponseNoContent.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kResponseNoContent.size());
        }

        return olp::http::SendOutcome(request_id);
      });

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
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT, s.GetStatus());
  EXPECT_EQ(kErrorNoContent, response.GetResult().GetErrorResponse().message);
}

TEST_F(AuthenticationClientTest, SignInFacebookData) {
  AuthenticationCredentials credentials(key_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(1)
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << kFacebookSigninResponse;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::OK)
                     .WithError(kErrorOk));
        if (data_callback) {
          auto raw = const_cast<char*>(kFacebookSigninResponse.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kFacebookSigninResponse.size());
        }

        return olp::http::SendOutcome(request_id);
      });

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
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response.GetResult().GetStatus());
  EXPECT_EQ(kErrorOk, response.GetResult().GetErrorResponse().message);
  EXPECT_EQ("facebook_grant_token", response.GetResult().GetAccessToken());
  EXPECT_GE(now + kMaxExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinExpiryTime, response.GetResult().GetExpiryTime());
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

TEST_F(AuthenticationClientTest, SignInGoogleData) {
  AuthenticationCredentials credentials(key_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(1)
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << kGoogleSigninResponse;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::OK)
                     .WithError(kErrorOk));
        if (data_callback) {
          auto raw = const_cast<char*>(kGoogleSigninResponse.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kGoogleSigninResponse.size());
        }

        return olp::http::SendOutcome(request_id);
      });

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
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response.GetResult().GetStatus());
  EXPECT_EQ(kErrorOk, response.GetResult().GetErrorResponse().message);
  EXPECT_EQ("google_grant_token", response.GetResult().GetAccessToken());
  EXPECT_GE(now + kMaxExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_FALSE(response.GetResult().GetRefreshToken().empty());
  EXPECT_FALSE(response.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationClientTest, SignInArcGisData) {
  AuthenticationCredentials credentials(key_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(1)
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << kArcgisSigninResponse;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::OK)
                     .WithError(kErrorOk));
        if (data_callback) {
          auto raw = const_cast<char*>(kArcgisSigninResponse.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kArcgisSigninResponse.size());
        }

        return olp::http::SendOutcome(request_id);
      });

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
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response.GetResult().GetStatus());
  EXPECT_EQ(kErrorOk, response.GetResult().GetErrorResponse().message);
  EXPECT_EQ("arcgis_grant_token", response.GetResult().GetAccessToken());
  EXPECT_GE(now + kMaxExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinExpiryTime, response.GetResult().GetExpiryTime());
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

TEST_F(AuthenticationClientTest, SignInRefreshData) {
  AuthenticationCredentials credentials(key_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(1)
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << kRefreshSigninResponse;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::OK)
                     .WithError(kErrorOk));
        if (data_callback) {
          auto raw = const_cast<char*>(kRefreshSigninResponse.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kRefreshSigninResponse.size());
        }

        return olp::http::SendOutcome(request_id);
      });

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
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response.GetResult().GetStatus());
  EXPECT_EQ(kErrorOk, response.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_EQ("refresh_grant_token", response.GetResult().GetAccessToken());
  EXPECT_GE(now + kMaxExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_FALSE(response.GetResult().GetRefreshToken().empty());
  EXPECT_FALSE(response.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationClientTest, ErrorFieldsData) {
  AuthenticationCredentials credentials(key_, secret_);
  std::promise<AuthenticationClient::SignInUserResponse> request;
  auto request_future = request.get_future();

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(3)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << kResponseErrorFields;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::BAD_REQUEST)
                     .WithError(kErrorFieldsMessage));
        if (data_callback) {
          auto raw = const_cast<char*>(kResponseErrorFields.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kResponseErrorFields.size());
        }

        return olp::http::SendOutcome(request_id);
      });

  AuthenticationClient::UserProperties properties;
  client_->SignInHereUser(
      credentials, properties,
      [&](const AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInUserResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
            response.GetResult().GetStatus());
  EXPECT_EQ(kErrorFieldsCode, response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorFieldsMessage,
            response.GetResult().GetErrorResponse().message);
  TestCheckErrorFields(response.GetResult().GetErrorFields());

  AuthenticationClient::SignOutUserResponse signOutResponse =
      SignOutUser("token");
  EXPECT_TRUE(signOutResponse.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
            signOutResponse.GetResult().GetStatus());
  EXPECT_EQ(kErrorFieldsCode,
            signOutResponse.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorFieldsMessage,
            signOutResponse.GetResult().GetErrorResponse().message);
  TestCheckErrorFields(response.GetResult().GetErrorFields());

  AuthenticationClient::SignUpResponse signup_response = SignUpUser("email");
  EXPECT_TRUE(signup_response.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
            signup_response.GetResult().GetStatus());
  EXPECT_EQ(kErrorFieldsCode,
            signup_response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorFieldsMessage,
            signup_response.GetResult().GetErrorResponse().message);
  TestCheckErrorFields(response.GetResult().GetErrorFields());
}

TEST_F(AuthenticationClientTest, TestInvalidResponses) {
  ExecuteSigninRequest(olp::http::HttpStatusCode::OK,
                       olp::http::HttpStatusCode::SERVICE_UNAVAILABLE,
                       kErrorServiceUnavailable, kResponseInvalidJson);
  ExecuteSigninRequest(olp::http::HttpStatusCode::OK,
                       olp::http::HttpStatusCode::SERVICE_UNAVAILABLE,
                       kErrorServiceUnavailable, kResponseNoToken);
  ExecuteSigninRequest(olp::http::HttpStatusCode::OK,
                       olp::http::HttpStatusCode::SERVICE_UNAVAILABLE,
                       kErrorServiceUnavailable, kResponseNoTokenType);
  ExecuteSigninRequest(olp::http::HttpStatusCode::OK,
                       olp::http::HttpStatusCode::SERVICE_UNAVAILABLE,
                       kErrorServiceUnavailable, kResponseNoExpiry);
}

TEST_F(AuthenticationClientTest, TestHttpRequestErrorCodes) {
  ExecuteSigninRequest(olp::http::HttpStatusCode::ACCEPTED,
                       olp::http::HttpStatusCode::ACCEPTED, kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::CREATED,
                       olp::http::HttpStatusCode::CREATED, kErrorUndefined,
                       kResponseCreated);
  ExecuteSigninRequest(olp::http::HttpStatusCode::NON_AUTHORITATIVE_INFORMATION,
                       olp::http::HttpStatusCode::NON_AUTHORITATIVE_INFORMATION,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::NO_CONTENT,
                       olp::http::HttpStatusCode::NO_CONTENT, kErrorUndefined,
                       kResponseNoContent);
  ExecuteSigninRequest(olp::http::HttpStatusCode::RESET_CONTENT,
                       olp::http::HttpStatusCode::RESET_CONTENT,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::PARTIAL_CONTENT,
                       olp::http::HttpStatusCode::PARTIAL_CONTENT,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::MULTIPLE_CHOICES,
                       olp::http::HttpStatusCode::MULTIPLE_CHOICES,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::MOVED_PERMANENTLY,
                       olp::http::HttpStatusCode::MOVED_PERMANENTLY,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::FOUND,
                       olp::http::HttpStatusCode::FOUND, kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::SEE_OTHER,
                       olp::http::HttpStatusCode::SEE_OTHER, kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::NOT_MODIFIED,
                       olp::http::HttpStatusCode::NOT_MODIFIED,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::USE_PROXY,
                       olp::http::HttpStatusCode::USE_PROXY, kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::BAD_REQUEST,
                       olp::http::HttpStatusCode::BAD_REQUEST,
                       kErrorBadRequestMessage, kResponseBadRequest,
                       kErrorBadRequestCode);
  ExecuteSigninRequest(olp::http::HttpStatusCode::UNAUTHORIZED,
                       olp::http::HttpStatusCode::UNAUTHORIZED,
                       kErrorUnathorizedMessage, kResponseUnauthorized,
                       kErrorUnauthorizedCode);
  ExecuteSigninRequest(olp::http::HttpStatusCode::PAYMENT_REQUIRED,
                       olp::http::HttpStatusCode::PAYMENT_REQUIRED,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::NOT_FOUND,
                       olp::http::HttpStatusCode::NOT_FOUND, kErrorUserNotFound,
                       kResponseNotFound, kErrorNotFoundCode);
  ExecuteSigninRequest(olp::http::HttpStatusCode::METHOD_NOT_ALLOWED,
                       olp::http::HttpStatusCode::METHOD_NOT_ALLOWED,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::FORBIDDEN,
                       olp::http::HttpStatusCode::FORBIDDEN, kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::NOT_ACCEPTABLE,
                       olp::http::HttpStatusCode::NOT_ACCEPTABLE,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::PROXY_AUTHENTICATION_REQUIRED,
                       olp::http::HttpStatusCode::PROXY_AUTHENTICATION_REQUIRED,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::REQUEST_TIMEOUT,
                       olp::http::HttpStatusCode::REQUEST_TIMEOUT,
                       kErrorUndefined);
  ExecuteSigninRequest(
      olp::http::HttpStatusCode::CONFLICT, olp::http::HttpStatusCode::CONFLICT,
      kErrorConfliceMessage, kResponseConflict, kErrorConfliceCode);
  ExecuteSigninRequest(olp::http::HttpStatusCode::GONE,
                       olp::http::HttpStatusCode::GONE, kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::LENGTH_REQUIRED,
                       olp::http::HttpStatusCode::LENGTH_REQUIRED,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::PRECONDITION_FAILED,
                       olp::http::HttpStatusCode::PRECONDITION_FAILED,
                       kErrorPreconditionFailedMessage,
                       kResponsePreconditionFailed);
  ExecuteSigninRequest(olp::http::HttpStatusCode::REQUEST_ENTITY_TOO_LARGE,
                       olp::http::HttpStatusCode::REQUEST_ENTITY_TOO_LARGE,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::REQUEST_URI_TOO_LONG,
                       olp::http::HttpStatusCode::REQUEST_URI_TOO_LONG,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::UNSUPPORTED_MEDIA_TYPE,
                       olp::http::HttpStatusCode::UNSUPPORTED_MEDIA_TYPE,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::TOO_MANY_REQUESTS,
                       olp::http::HttpStatusCode::TOO_MANY_REQUESTS,
                       kErrorTooManyRequestsMessage, kResponseTooManyRequests,
                       kErrorTooManyRequestsCode);
  ExecuteSigninRequest(olp::http::HttpStatusCode::INTERNAL_SERVER_ERROR,
                       olp::http::HttpStatusCode::INTERNAL_SERVER_ERROR,
                       kErrorInternvalServerMessage,
                       kResponseInternalServerError, kErrorInternalServerCode);
  ExecuteSigninRequest(olp::http::HttpStatusCode::NOT_IMPLEMENTED,
                       olp::http::HttpStatusCode::NOT_IMPLEMENTED,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::BAD_GATEWAY,
                       olp::http::HttpStatusCode::BAD_GATEWAY, kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::SERVICE_UNAVAILABLE,
                       olp::http::HttpStatusCode::SERVICE_UNAVAILABLE,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::GATEWAY_TIMEOUT,
                       olp::http::HttpStatusCode::GATEWAY_TIMEOUT,
                       kErrorUndefined);
  ExecuteSigninRequest(olp::http::HttpStatusCode::VERSION_NOT_SUPPORTED,
                       olp::http::HttpStatusCode::VERSION_NOT_SUPPORTED,
                       kErrorUndefined);
  ExecuteSigninRequest(100000, 100000, kErrorUndefined);
  ExecuteSigninRequest(-100000, -100000, kErrorUndefined);
}
