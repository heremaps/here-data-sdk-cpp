/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/NetworkMock.h>
#include <olp/authentication/AuthenticationClient.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/NetworkConstants.h>
#include <olp/core/http/NetworkUtils.h>
#include <olp/core/porting/make_unique.h>

#include <future>
#include <memory>

#include "AuthenticationMockedResponses.h"
#include "olp/core/client/OlpClientSettingsFactory.h"

namespace auth = olp::authentication;
namespace client = olp::client;
using testing::_;

namespace {
constexpr auto kTimestampUrl =
    R"(https://authentication.server.url/timestamp)";

constexpr auto kIntrospectUrl =
    R"(https://authentication.server.url/app/me)";

constexpr auto kTokenEndpointUrl = "https://authentication.server.url";

constexpr unsigned int kExpirtyTime = 3600;
constexpr unsigned int kMaxExpiryTime = kExpirtyTime + 30;
constexpr unsigned int kMinExpiryTime = kExpirtyTime - 10;

// HTTP errors
constexpr auto kErrorOk = "OK";
constexpr auto kErrorSignupCreated = "Created";
constexpr auto kErrorServiceUnavailable = "Service unavailable";

constexpr auto kErrorNoContent = "No Content";
constexpr auto kErrorFieldsMessage = "Received invalid data.";
constexpr auto kErrorPreconditionFailedMessage = "Precondition Failed";

constexpr auto kErrorBadRequestMessage = "Invalid JSON.";

constexpr auto kErrorUnathorizedMessage =
    "Signature mismatch. Authorization signature or client credential is "
    "wrong.";

constexpr auto kErrorUserNotFound =
    "User for the given access token cannot be found.";

constexpr auto kErrorConfliceMessage =
    "A password account with the specified email address already exists.";

constexpr auto kErrorTooManyRequestsMessage =
    "Request blocked because too many requests were made. Please wait for a "
    "while before making a new request.";

constexpr auto kErrorInternvalServerMessage = "Missing Thing Encrypted Secret.";

constexpr auto kErrorIllegalLastName = "Illegal last name.";
constexpr auto kErrorBlacklistedPassword = "Black listed password.";
constexpr auto kDateHeader = "Fri, 29 May 2020 11:07:45 GMT";
constexpr auto kRequestAuth = "https://authentication.server.url/oauth2/token";
constexpr auto kDate = "date";

constexpr auto kErrorFieldsCode = 400200;
constexpr auto kErrorBadRequestCode = 400002;
constexpr auto kErrorIllegalLastNameCode = 400203;
constexpr auto kErrorBlacklistedPasswordCode = 400205;
constexpr auto kErrorTooManyRequestsCode = 429002;
constexpr auto kErrorUnauthorizedCode = 401300;
constexpr auto kErrorNotFoundCode = 404000;
constexpr auto kErrorConfliceCode = 409100;
constexpr auto kErrorInternalServerCode = 500203;

void TestCheckErrorFields(const auth::ErrorFields& errorFields) {
  static const std::string kPassword = "password";
  static const std::string kLastName = "lastname";
  static const std::string kEmail = "email";

  EXPECT_EQ(2, errorFields.size());
  int count = 0;
  for (auto error_it = errorFields.begin(); error_it != errorFields.end();
       error_it++) {
    const std::string name = count == 0 ? kPassword : kLastName;
    const std::string message =
        count == 0 ? kErrorBlacklistedPassword : kErrorIllegalLastName;
    const unsigned int code =
        count == 0 ? kErrorBlacklistedPasswordCode : kErrorIllegalLastNameCode;
    count++;
    EXPECT_EQ(name, error_it->name);
    EXPECT_EQ(message, error_it->message);
    EXPECT_EQ(code, error_it->code);
  }
}
}  // namespace

class AuthenticationClientTest : public ::testing::Test {
 public:
  AuthenticationClientTest()
      : key_("key"), secret_("secret"), scope_("scope") {}
  void SetUp() {
    network_ = std::make_shared<NetworkMock>();
    task_scheduler_ =
        client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();

    auth::AuthenticationSettings settings;
    settings.network_request_handler = network_;
    settings.task_scheduler = task_scheduler_;
    settings.token_endpoint_url = kTokenEndpointUrl;
    settings.use_system_time = false;
    client_ = std::make_unique<auth::AuthenticationClient>(settings);
  }

  void TearDown() {}

  auth::AuthenticationClient::SignUpResponse SignUpUser(
      const std::string& email) {
    auth::AuthenticationCredentials credentials(key_, secret_);
    std::promise<auth::AuthenticationClient::SignUpResponse> request;
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
        [&](const auth::AuthenticationClient::SignUpResponse& response) {
          request.set_value(response);
        });

    request_future.wait();
    return request_future.get();
  }

  auth::AuthenticationClient::SignOutUserResponse SignOutUser(
      const std::string& access_token) {
    auth::AuthenticationCredentials credentials(key_, secret_);
    std::promise<auth::AuthenticationClient::SignOutUserResponse> request;
    auto request_future = request.get_future();
    auto cancel_token = client_->SignOut(
        credentials, access_token,
        [&](const auth::AuthenticationClient::SignOutUserResponse& response) {
          request.set_value(response);
        });

    request_future.wait();
    return request_future.get();
  }

  void ExecuteSigninRequest(int http, int http_result,
                            const std::string& error_message,
                            const std::string& data = "", int error_code = 0) {
    auth::AuthenticationCredentials credentials(key_, secret_);
    std::promise<auth::AuthenticationClient::SignInClientResponse> request;
    auto request_future = request.get_future();

    const bool is_retriable = olp::client::DefaultRetryCondition({http});

    // First is GetTimeFromServer(). Second is actual SignIn.
    // When the request is retriable, 3 more requests are fired.
    const auto expected_number_of_calls = is_retriable ? 8 : 2;

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .Times(expected_number_of_calls)
        .WillRepeatedly(
            [&](olp::http::NetworkRequest /*request*/,
                olp::http::Network::Payload payload,
                olp::http::Network::Callback callback,
                olp::http::Network::HeaderCallback /*header_callback*/,
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
        credentials, {},
        [&](const auth::AuthenticationClient::SignInClientResponse& response) {
          request.set_value(response);
        });
    request_future.wait();
    auth::AuthenticationClient::SignInClientResponse response =
        request_future.get();
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
  const std::string scope_;
};

TEST_F(AuthenticationClientTest, DefaultTimeSource) {
  // Default time source should be system time
  auth::AuthenticationSettings settings;
  settings.network_request_handler = network_;
  settings.token_endpoint_url = kTokenEndpointUrl;
  settings.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
  ASSERT_TRUE(settings.use_system_time);

  EXPECT_CALL(*network_, Send(IsGetRequest(kTimestampUrl), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                                   kResponseWithScope));

  std::promise<void> request;
  auth::AuthenticationClient::SignInClientResponse response;
  auth::AuthenticationClient::SignInProperties properties;
  auto client =
      std::make_unique<auth::AuthenticationClient>(std::move(settings));

  client->SignInClient(
      auth::AuthenticationCredentials(key_, secret_), properties,
      [&](const auth::AuthenticationClient::SignInClientResponse& value) {
        response = value;
        request.set_value();
      });

  request.get_future().wait();
  EXPECT_TRUE(response.IsSuccessful());

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, SignInClientUseLocalTime) {
  auth::AuthenticationSettings settings;
  settings.network_request_handler = network_;
  settings.use_system_time = true;
  settings.token_endpoint_url = kTokenEndpointUrl;
  settings.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();

  auto client =
      std::make_unique<auth::AuthenticationClient>(std::move(settings));

  EXPECT_CALL(*network_, Send(IsGetRequest(kTimestampUrl), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                                   kResponseWithScope));

  auth::AuthenticationClient::SignInProperties properties;
  properties.scope = scope_;
  std::time_t now = std::time(nullptr);
  std::promise<auth::AuthenticationClient::SignInClientResponse> request;

  client->SignInClient(
      auth::AuthenticationCredentials(key_, secret_), properties,
      [&](const auth::AuthenticationClient::SignInClientResponse& response) {
        request.set_value(response);
      });

  auto request_future = request.get_future();
  auto response = request_future.get();

  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_EQ(kResponseToken, response.GetResult().GetAccessToken());
  EXPECT_GE(now + kMaxExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());
  EXPECT_EQ(response.GetResult().GetScope(), scope_);

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, SignInClientUseWrongLocalTime) {
  auth::AuthenticationSettings settings;
  settings.network_request_handler = network_;
  settings.use_system_time = true;
  settings.token_endpoint_url = kTokenEndpointUrl;
  settings.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();

  auto client =
      std::make_unique<auth::AuthenticationClient>(std::move(settings));

  auth::AuthenticationCredentials credentials(key_, secret_);
  std::promise<auth::AuthenticationClient::SignInClientResponse> request;
  auto request_future = request.get_future();

  EXPECT_CALL(*network_, Send(IsGetRequest(kTimestampUrl), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest /*request*/,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << kResponseWrongTimestamp;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::UNAUTHORIZED));
        if (data_callback) {
          auto raw = const_cast<char*>(kResponseWrongTimestamp.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kResponseWrongTimestamp.size());
        }
        if (header_callback) {
          header_callback(kDate, kDateHeader);
        }

        return olp::http::SendOutcome(request_id);
      })
      .WillOnce(ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                                   kResponseWithScope));

  auth::AuthenticationClient::SignInProperties properties;
  properties.scope = scope_;
  std::time_t now = std::time(nullptr);
  client->SignInClient(
      credentials, properties,
      [&](const auth::AuthenticationClient::SignInClientResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  auth::AuthenticationClient::SignInClientResponse response =
      request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_EQ(kResponseToken, response.GetResult().GetAccessToken());
  EXPECT_GE(now + kMaxExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());
  EXPECT_EQ(response.GetResult().GetScope(), scope_);
}

TEST_F(AuthenticationClientTest, SignInClientScope) {
  EXPECT_CALL(*network_, Send(IsGetRequest(kTimestampUrl), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                                   kResponseTime));

  EXPECT_CALL(*network_,
              Send(testing::Not(IsGetRequest(kTimestampUrl)), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                                   kResponseWithScope));

  auth::AuthenticationClient::SignInProperties properties;
  properties.scope = scope_;
  std::time_t now = std::time(nullptr);
  auth::AuthenticationCredentials credentials(key_, secret_);
  std::promise<auth::AuthenticationClient::SignInClientResponse> request;

  client_->SignInClient(
      credentials, properties,
      [&](const auth::AuthenticationClient::SignInClientResponse& response) {
        request.set_value(response);
      });

  auto request_future = request.get_future();
  auto response = request_future.get();

  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_EQ(kResponseToken, response.GetResult().GetAccessToken());
  EXPECT_GE(now + kMaxExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinExpiryTime, response.GetResult().GetExpiryTime());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());
  EXPECT_EQ(response.GetResult().GetScope(), scope_);

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, SignInClientData) {
  auth::AuthenticationCredentials credentials("key_", secret_);
  std::time_t now = std::time(nullptr);

  EXPECT_CALL(*network_, Send(IsGetRequest(kTimestampUrl), _, _, _, _))
      .Times(2)
      .WillRepeatedly(ReturnHttpResponse(
          GetResponse(olp::http::HttpStatusCode::OK), kResponseTime));

  EXPECT_CALL(*network_,
              Send(testing::Not(IsGetRequest(kTimestampUrl)), _, _, _, _))
      .Times(2)
      .WillOnce(ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                                   kResponseValidJson))
      .WillOnce([&](olp::http::NetworkRequest /*request*/,
                    olp::http::Network::Payload /*payload*/,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        olp::http::RequestId request_id(6);
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(-1)
                     .WithError(""));

        return olp::http::SendOutcome(request_id);
      });

  {
    SCOPED_TRACE("First request");

    std::promise<auth::AuthenticationClient::SignInClientResponse> request;

    client_->SignInClient(
        credentials, {},
        [&](const auth::AuthenticationClient::SignInClientResponse& response) {
          request.set_value(response);
        });

    auto request_future = request.get_future();
    auto response = request_future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
    EXPECT_EQ(kResponseToken, response.GetResult().GetAccessToken());
    EXPECT_GE(now + kMaxExpiryTime, response.GetResult().GetExpiryTime());
    EXPECT_LT(now + kMinExpiryTime, response.GetResult().GetExpiryTime());
    EXPECT_EQ("bearer", response.GetResult().GetTokenType());
    EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
    EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());
  }

  {
    SCOPED_TRACE("Second request");

    std::promise<auth::AuthenticationClient::SignInClientResponse> request;
    now = std::time(nullptr);

    client_->SignInClient(
        credentials, {},
        [&](const auth::AuthenticationClient::SignInClientResponse& response) {
          request.set_value(response);
        });

    auto request_future = request.get_future();
    auto response = request_future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(kResponseToken, response.GetResult().GetAccessToken());
    EXPECT_GE(now + kMaxExpiryTime, response.GetResult().GetExpiryTime());
    EXPECT_LT(now + kMinExpiryTime, response.GetResult().GetExpiryTime());
    EXPECT_EQ("bearer", response.GetResult().GetTokenType());
    EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
    EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());
  }

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, SignUpHereUserData) {
  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(1)
      .WillOnce([&](olp::http::NetworkRequest /*request*/,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
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

  auto response = SignUpUser("email@example.com");

  EXPECT_TRUE(response.IsSuccessful());
  const auto& result = response.GetResult();
  EXPECT_EQ(olp::http::HttpStatusCode::CREATED, result.GetStatus());
  EXPECT_EQ(kErrorSignupCreated, result.GetErrorResponse().message);
  EXPECT_FALSE(result.GetUserIdentifier().empty());

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, SignInUserDataFirstTime) {
  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(olp::http::HttpStatusCode::PRECONDITION_FAILED)
              .WithError(kErrorPreconditionFailedMessage),
          kSigninUserFirstTimeResponse));

  auth::AuthenticationClient::UserProperties properties;
  std::promise<auth::AuthenticationClient::SignInUserResponse> request;

  client_->SignInHereUser(
      auth::AuthenticationCredentials(key_, secret_), properties,
      [&](const auth::AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });

  auto request_future = request.get_future();
  auto response = request_future.get();

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

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, AcceptTermsData) {
  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .WillOnce(
          ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::NO_CONTENT)
                                 .WithError(kErrorNoContent),
                             kResponseNoContent));

  std::promise<auth::AuthenticationClient::SignInUserResponse> request;

  client_->AcceptTerms(
      auth::AuthenticationCredentials(key_, secret_), "reacceptance_token",
      [&](const auth::AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });

  auto request_future = request.get_future();
  auto response = request_future.get();

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

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, SignInHereUser) {
  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(olp::http::HttpStatusCode::OK).WithError(kErrorOk),
          kUserSigninResponse));

  auth::AuthenticationClient::UserProperties properties;
  std::time_t now = std::time(nullptr);
  std::promise<auth::AuthenticationClient::SignInUserResponse> request;

  client_->SignInHereUser(
      auth::AuthenticationCredentials(key_, secret_), properties,
      [&](const auth::AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });

  auto request_future = request.get_future();
  auto response = request_future.get();

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

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, SignOutUser) {
  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .WillOnce(
          ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::NO_CONTENT)
                                 .WithError(kErrorNoContent),
                             kResponseNoContent));

  std::promise<auth::AuthenticationClient::SignOutUserResponse> request;

  client_->SignOut(
      auth::AuthenticationCredentials(key_, secret_),
      "h1.C33vsPr8atTZcXOC7AWbgQ.hCGWE5CNLuQv4vSLJUOAqGuRNjhO34qCH8mZIQ-"
      "93gBqlf34y37DNl92FUnPrgECxojv7rn4bXYRZDohlx1o91bMgQH20G2N94bdrl2pOB9XT"
      "_"
      "rqT54anW_XfGZAZQRwPz8RRayuNBcf_FGDFyn0YFP0_"
      "c4tH8yg6HI0YK5sL7N09JRhoBKdhseBH_"
      "QOiR2IYJsvlhcjkdduHHWFQQpDNPwlqRLJ9ivfwSVmsxIgxra2x85bxdkg1kY-"
      "H5ZeBIFkyxs6E__"
      "OT5aYPWhOoR2aqdtVUWtcQDuqccEKzXDcs8dYVKgU7jxyUG6GspW67397EK-"
      "XOPbk9IqTPNpOFOioVeZn1ylw5FuriUpsxAUX8VS7DOldw5mQ-"
      "OyE91MDGvItecI6PmRDSdyE5c9xTQ759vY07sUStP0K-Cq65UKqwysN_"
      "3qSvgqcFotalyUMbtYoW0DGquS7aORdK0azI2LT2Q."
      "bpjVuX3Zr69to7dHhZoTXGulAmLv_"
      "ES4Ne1d3bQ7XiWVggDeRGzZvg-49P0cTz146aV7ugl71-"
      "opH2ATwLFekphRd8NaYcc2aVKo4stZgBr6ZVvO9HKqvZZ02lPbQXJuRqt1yEmEpLIMJbD-"
      "o8M8_"
      "Im2mE_NbivYDZkYSzz-"
      "pIw5c0qHluBFF3e8QSVU99dNOBLrHTQ51j3qejLQ3q8DQzKYfg3EMMstVH6VC4xvWabn0a"
      "3-"
      "TQHbrQ-P_h4Ei5oP10Kmhur-lGmMBomAaByHWulqTyv19RXvAIC4rg_b2OYA-"
      "uzPwcDGeDB5h24l08Cgxq7r7mPKcwSgTOHZY4oaaA",
      [&](const auth::AuthenticationClient::SignOutUserResponse& response) {
        request.set_value(response);
      });

  auto request_future = request.get_future();
  auto response = request_future.get();

  EXPECT_TRUE(response.IsSuccessful());
  auto result = response.MoveResult();
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT, result.GetStatus());
  EXPECT_EQ(kErrorNoContent, result.GetErrorResponse().message);

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, SignInFederated) {
  std::string body =
      R"({ "grantType": "xyz", "token": "test_token", "realm": "my_realm" })";

  EXPECT_CALL(*network_,
              Send(testing::AllOf(HeadersContainAuthorization(), BodyEq(body)),
                   _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(olp::http::HttpStatusCode::OK).WithError(kErrorOk),
          kUserSigninResponse));

  std::promise<auth::AuthenticationClient::SignInUserResponse> request;

  client_->SignInFederated(
      auth::AuthenticationCredentials(key_, secret_), body,
      [&](const auth::AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });

  auto request_future = request.get_future();
  auto response = request_future.get();

  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response.GetResult().GetStatus());
  EXPECT_EQ(kErrorOk, response.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_EQ("password_grant_token", response.GetResult().GetAccessToken());
  EXPECT_EQ("bearer", response.GetResult().GetTokenType());
  EXPECT_LT(0ll, response.GetResult().GetExpiresIn().count());
  EXPECT_FALSE(response.GetResult().GetRefreshToken().empty());
  EXPECT_FALSE(response.GetResult().GetUserIdentifier().empty());

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, SignInFacebookData) {
  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(olp::http::HttpStatusCode::OK).WithError(kErrorOk),
          kFacebookSigninResponse));

  auth::AuthenticationClient::FederatedProperties properties;
  std::time_t now = std::time(nullptr);
  std::promise<auth::AuthenticationClient::SignInUserResponse> request;

  client_->SignInFacebook(
      auth::AuthenticationCredentials(key_, secret_), properties,
      [&](const auth::AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });

  auto request_future = request.get_future();
  auto response = request_future.get();

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

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, SignInArcGisData) {
  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(olp::http::HttpStatusCode::OK).WithError(kErrorOk),
          kArcgisSigninResponse));

  auth::AuthenticationClient::FederatedProperties properties;
  std::time_t now = std::time(nullptr);
  std::promise<auth::AuthenticationClient::SignInUserResponse> request;

  client_->SignInArcGis(
      auth::AuthenticationCredentials(key_, secret_), properties,
      [&](const auth::AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });

  auto request_future = request.get_future();
  auto response = request_future.get();

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

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, SignInRefreshData) {
  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(olp::http::HttpStatusCode::OK).WithError(kErrorOk),
          kRefreshSigninResponse));

  auth::AuthenticationClient::UserProperties properties;
  std::time_t now = std::time(nullptr);
  std::promise<auth::AuthenticationClient::SignInUserResponse> request;

  client_->SignInHereUser(
      auth::AuthenticationCredentials(key_, secret_), properties,
      [&](const auth::AuthenticationClient::SignInUserResponse& response) {
        request.set_value(response);
      });

  auto request_future = request.get_future();
  auto response = request_future.get();

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

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, ErrorFieldsData) {
  {
    SCOPED_TRACE("SignInHereUser");

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .Times(3)
        .WillRepeatedly(
            [&](olp::http::NetworkRequest /*request*/,
                olp::http::Network::Payload payload,
                olp::http::Network::Callback callback,
                olp::http::Network::HeaderCallback /*header_callback*/,
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

    auth::AuthenticationClient::UserProperties properties;
    std::promise<auth::AuthenticationClient::SignInUserResponse> request;

    client_->SignInHereUser(
        auth::AuthenticationCredentials(key_, secret_), properties,
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          request.set_value(response);
        });

    auto request_future = request.get_future();
    auto response = request_future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetResult().GetStatus());
    EXPECT_EQ(kErrorFieldsCode, response.GetResult().GetErrorResponse().code);
    EXPECT_EQ(kErrorFieldsMessage,
              response.GetResult().GetErrorResponse().message);
    TestCheckErrorFields(response.GetResult().GetErrorFields());
  }

  {
    SCOPED_TRACE("SignOutUser");

    auto signout_response = SignOutUser("token");
    EXPECT_TRUE(signout_response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              signout_response.GetResult().GetStatus());
    EXPECT_EQ(kErrorFieldsCode,
              signout_response.GetResult().GetErrorResponse().code);
    EXPECT_EQ(kErrorFieldsMessage,
              signout_response.GetResult().GetErrorResponse().message);
    TestCheckErrorFields(signout_response.GetResult().GetErrorFields());
  }

  {
    SCOPED_TRACE("SignUpUser");

    auto signup_response = SignUpUser("email");
    EXPECT_TRUE(signup_response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              signup_response.GetResult().GetStatus());
    EXPECT_EQ(kErrorFieldsCode,
              signup_response.GetResult().GetErrorResponse().code);
    EXPECT_EQ(kErrorFieldsMessage,
              signup_response.GetResult().GetErrorResponse().message);
    TestCheckErrorFields(signup_response.GetResult().GetErrorFields());
  }

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, TestInvalidResponses) {
  {
    SCOPED_TRACE("Invalid JSON");

    ExecuteSigninRequest(olp::http::HttpStatusCode::OK,
                         olp::http::HttpStatusCode::SERVICE_UNAVAILABLE,
                         kErrorServiceUnavailable, kResponseInvalidJson);
  }

  {
    SCOPED_TRACE("No token");

    ExecuteSigninRequest(olp::http::HttpStatusCode::OK,
                         olp::http::HttpStatusCode::SERVICE_UNAVAILABLE,
                         kErrorServiceUnavailable, kResponseNoToken);
  }

  {
    SCOPED_TRACE("Token type missing");

    ExecuteSigninRequest(olp::http::HttpStatusCode::OK,
                         olp::http::HttpStatusCode::SERVICE_UNAVAILABLE,
                         kErrorServiceUnavailable, kResponseNoTokenType);
  }

  {
    SCOPED_TRACE("Missing expiry");

    ExecuteSigninRequest(olp::http::HttpStatusCode::OK,
                         olp::http::HttpStatusCode::SERVICE_UNAVAILABLE,
                         kErrorServiceUnavailable, kResponseNoExpiry);
  }

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, TestHttpRequestErrorCodes) {
  auto status = olp::http::HttpStatusCode::OK;

  {
    SCOPED_TRACE("ACCEPTED");

    status = olp::http::HttpStatusCode::ACCEPTED;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("CREATED");

    status = olp::http::HttpStatusCode::CREATED;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status),
                         kResponseCreated);
  }

  {
    SCOPED_TRACE("NON_AUTHORITATIVE_INFORMATION");

    status = olp::http::HttpStatusCode::NON_AUTHORITATIVE_INFORMATION;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("NO_CONTENT");

    status = olp::http::HttpStatusCode::NO_CONTENT;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status),
                         kResponseNoContent);
  }

  {
    SCOPED_TRACE("RESET_CONTENT");

    status = olp::http::HttpStatusCode::RESET_CONTENT;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("MULTIPLE_CHOICES");

    status = olp::http::HttpStatusCode::PARTIAL_CONTENT;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("MULTIPLE_CHOICES");

    status = olp::http::HttpStatusCode::MULTIPLE_CHOICES;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("MOVED_PERMANENTLY");

    status = olp::http::HttpStatusCode::MOVED_PERMANENTLY;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("FOUND");

    status = olp::http::HttpStatusCode::FOUND;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("SEE_OTHER");

    status = olp::http::HttpStatusCode::SEE_OTHER;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("NOT_MODIFIED");

    status = olp::http::HttpStatusCode::NOT_MODIFIED;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("USE_PROXY");

    status = olp::http::HttpStatusCode::USE_PROXY;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("BAD_REQUEST");

    status = olp::http::HttpStatusCode::BAD_REQUEST;
    ExecuteSigninRequest(status, status, kErrorBadRequestMessage,
                         kResponseBadRequest, kErrorBadRequestCode);
  }

  {
    SCOPED_TRACE("UNAUTHORIZED");

    status = olp::http::HttpStatusCode::UNAUTHORIZED;
    ExecuteSigninRequest(status, status, kErrorUnathorizedMessage,
                         kResponseUnauthorized, kErrorUnauthorizedCode);
  }

  {
    SCOPED_TRACE("PAYMENT_REQUIRED");

    status = olp::http::HttpStatusCode::PAYMENT_REQUIRED;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("NOT_FOUND");

    status = olp::http::HttpStatusCode::NOT_FOUND;
    ExecuteSigninRequest(status, status, kErrorUserNotFound, kResponseNotFound,
                         kErrorNotFoundCode);
  }

  {
    SCOPED_TRACE("METHOD_NOT_ALLOWED");

    status = olp::http::HttpStatusCode::METHOD_NOT_ALLOWED;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("FORBIDDEN");

    status = olp::http::HttpStatusCode::FORBIDDEN;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("NOT_ACCEPTABLE");

    status = olp::http::HttpStatusCode::NOT_ACCEPTABLE;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("PROXY_AUTHENTICATION_REQUIRED");

    status = olp::http::HttpStatusCode::PROXY_AUTHENTICATION_REQUIRED;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("REQUEST_TIMEOUT");

    status = olp::http::HttpStatusCode::REQUEST_TIMEOUT;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("CONFLICT");

    status = olp::http::HttpStatusCode::CONFLICT;
    ExecuteSigninRequest(status, status, kErrorConfliceMessage,
                         kResponseConflict, kErrorConfliceCode);
  }

  {
    SCOPED_TRACE("GONE");

    status = olp::http::HttpStatusCode::GONE;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("LENGTH_REQUIRED");

    status = olp::http::HttpStatusCode::LENGTH_REQUIRED;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("PRECONDITION_FAILED");

    status = olp::http::HttpStatusCode::PRECONDITION_FAILED;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status),
                         kResponsePreconditionFailed);
  }

  {
    SCOPED_TRACE("REQUEST_ENTITY_TOO_LARGE");

    status = olp::http::HttpStatusCode::REQUEST_ENTITY_TOO_LARGE;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("REQUEST_URI_TOO_LONG");

    status = olp::http::HttpStatusCode::REQUEST_URI_TOO_LONG;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("UNSUPPORTED_MEDIA_TYPE");

    status = olp::http::HttpStatusCode::UNSUPPORTED_MEDIA_TYPE;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("TOO_MANY_REQUESTS");

    status = olp::http::HttpStatusCode::TOO_MANY_REQUESTS;
    ExecuteSigninRequest(status, status, kErrorTooManyRequestsMessage,
                         kResponseTooManyRequests, kErrorTooManyRequestsCode);
  }

  {
    SCOPED_TRACE("INTERNAL_SERVER_ERROR");

    status = olp::http::HttpStatusCode::INTERNAL_SERVER_ERROR;
    ExecuteSigninRequest(status, status, kErrorInternvalServerMessage,
                         kResponseInternalServerError,
                         kErrorInternalServerCode);
  }

  {
    SCOPED_TRACE("NOT_IMPLEMENTED");

    status = olp::http::HttpStatusCode::NOT_IMPLEMENTED;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("BAD_GATEWAY");

    status = olp::http::HttpStatusCode::BAD_GATEWAY;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("SERVICE_UNAVAILABLE");

    status = olp::http::HttpStatusCode::SERVICE_UNAVAILABLE;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("GATEWAY_TIMEOUT");

    status = olp::http::HttpStatusCode::GATEWAY_TIMEOUT;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("VERSION_NOT_SUPPORTED");

    status = olp::http::HttpStatusCode::VERSION_NOT_SUPPORTED;
    ExecuteSigninRequest(status, status, olp::http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("Custom positive status");

    auto custom_error = 100000;
    ExecuteSigninRequest(custom_error, custom_error,
                         olp::http::HttpErrorToString(custom_error));
  }

  {
    SCOPED_TRACE("Custom negative status");

    auto custom_error = -100000;
    ExecuteSigninRequest(custom_error, custom_error,
                         olp::http::HttpErrorToString(custom_error));
  }

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, IntrospectApp) {
  {
    SCOPED_TRACE("Successful request");

    EXPECT_CALL(*network_, Send(IsGetRequest(kIntrospectUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                                     kIntrospectAppResponse));

    std::promise<auth::IntrospectAppResponse> request;

    client_->IntrospectApp(kResponseToken,
                           [&](const auth::IntrospectAppResponse& response) {
                             request.set_value(response);
                           });

    auto future = request.get_future();
    auto response = future.get();
    auto result = response.GetResult();
    auto error = response.GetError();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_FALSE(result.GetClientId().empty());
    EXPECT_FALSE(result.GetName().empty());
    EXPECT_FALSE(result.GetDescription().empty());
    EXPECT_FALSE(result.GetRedirectUris().empty());
    EXPECT_FALSE(result.GetAllowedScopes().empty());
    EXPECT_FALSE(result.GetTokenEndpointAuthMethod().empty());
    EXPECT_FALSE(result.GetTokenEndpointAuthMethodReason().empty());
    EXPECT_FALSE(result.GetDobRequired());
    EXPECT_TRUE(result.GetTokenDuration() > 0);
    EXPECT_FALSE(result.GetReferrers().empty());
    EXPECT_FALSE(result.GetStatus().empty());
    EXPECT_TRUE(result.GetAppCodeEnabled());
    EXPECT_TRUE(result.GetCreatedTime() > 0);
    EXPECT_FALSE(result.GetRealm().empty());
    EXPECT_FALSE(result.GetType().empty());
    EXPECT_FALSE(result.GetResponseTypes().empty());
    EXPECT_FALSE(result.GetTier().empty());
    EXPECT_FALSE(result.GetHRN().empty());

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
  {
    SCOPED_TRACE("Invalid access token");

    EXPECT_CALL(*network_, Send(IsGetRequest(kIntrospectUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            GetResponse(olp::http::HttpStatusCode::UNAUTHORIZED),
            kInvalidAccessTokenResponse));

    std::promise<auth::IntrospectAppResponse> request;

    client_->IntrospectApp(kResponseToken,
                           [&](const auth::IntrospectAppResponse& response) {
                             request.set_value(response);
                           });

    auto future = request.get_future();
    auto response = future.get();
    auto error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::AccessDenied);

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
  {
    SCOPED_TRACE("Invalid response");

    EXPECT_CALL(*network_, Send(IsGetRequest(kIntrospectUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                                     "Invalid responce"));

    std::promise<auth::IntrospectAppResponse> request;

    client_->IntrospectApp(kResponseToken,
                           [&](const auth::IntrospectAppResponse& response) {
                             request.set_value(response);
                           });

    auto future = request.get_future();
    auto response = future.get();
    auto error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), olp::client::ErrorCode::Unknown);

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
}

TEST_F(AuthenticationClientTest, IntrospectAppCancel) {
  // Simulate a loaded queue
  std::promise<void> promise;
  auto future = promise.get_future();
  task_scheduler_->ScheduleTask([&future]() { future.get(); });

  std::promise<auth::IntrospectAppResponse> request;
  auto introspect_future = request.get_future();
  auto cancel_token = client_->IntrospectApp(
      kResponseToken, [&](const auth::IntrospectAppResponse& response) {
        request.set_value(response);
      });
  cancel_token.Cancel();
  promise.set_value();

  auto response = introspect_future.get();
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);

  testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, Authorize) {
  {
    SCOPED_TRACE("Successful request");

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                                     kAuthorizeResponseValid));

    auth::AuthorizeRequest authorize_request;
    std::promise<auth::AuthorizeResponse> request;

    client_->Authorize(kResponseToken, authorize_request,
                       [&](const auth::AuthorizeResponse& response) {
                         request.set_value(response);
                       });

    auto future = request.get_future();
    auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    auto result = response.GetResult();
    EXPECT_EQ(result.GetClientId(), "some_id");
    ASSERT_EQ(result.GetDecision(), olp::authentication::DecisionType::kAllow);

    auto it = result.GetActionResults().begin();
    ASSERT_EQ(it->GetDecision(), olp::authentication::DecisionType::kAllow);
    ASSERT_EQ(it->GetPermissions().front().GetAction(), "read");
    ASSERT_EQ(it->GetPermissions().front().GetResource(), "some_resource");
    ASSERT_EQ(it->GetPermissions().front().GetDecision(),
              olp::authentication::DecisionType::kAllow);

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
  {
    SCOPED_TRACE("Failed request");

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                                     kAuthorizeResponseError));

    auth::AuthorizeRequest authorize_request;
    std::promise<auth::AuthorizeResponse> request;

    client_->Authorize(kResponseToken, authorize_request,
                       [&](const auth::AuthorizeResponse& response) {
                         request.set_value(response);
                       });

    auto future = request.get_future();
    auto response = future.get();

    auto error = response.GetError();
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::Unknown);
    EXPECT_EQ(error.GetMessage(), "Error code: 409400");
  }
  {
    SCOPED_TRACE("Failed request error");

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            GetResponse(olp::http::HttpStatusCode::UNAUTHORIZED),
            kAuthorizeResponseErrorField));

    auth::AuthorizeRequest authorize_request;
    std::promise<auth::AuthorizeResponse> request;

    client_->Authorize(kResponseToken, authorize_request,
                       [&](const auth::AuthorizeResponse& response) {
                         request.set_value(response);
                       });

    auto future = request.get_future();
    auto response = future.get();

    auto error = response.GetError();
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::AccessDenied);
    EXPECT_EQ(error.GetMessage(), "Invalid client credentials.");

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
  {
    SCOPED_TRACE("Retry after failed network error");

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .Times(4)
        .WillRepeatedly(
            [&](olp::http::NetworkRequest /*request*/,
                olp::http::Network::Payload /*payload*/,
                olp::http::Network::Callback callback,
                olp::http::Network::HeaderCallback /*header_callback*/,
                olp::http::Network::DataCallback /*data_callback*/) {
              olp::http::RequestId request_id(3);

              callback(olp::http::NetworkResponse()
                           .WithRequestId(request_id)
                           .WithStatus(
                               olp::http::HttpStatusCode::SERVICE_UNAVAILABLE));
              return olp::http::SendOutcome(request_id);
            });

    auth::AuthorizeRequest authorize_request;
    std::promise<auth::AuthorizeResponse> request;
    auto future = request.get_future();
    client_->Authorize(kResponseToken, authorize_request,
                       [&](const auth::AuthorizeResponse& response) {
                         request.set_value(response);
                       });
    auth::AuthorizeResponse response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::ServiceUnavailable);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
  {
    SCOPED_TRACE("Failed corrupted responce");

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                                     "some_invalid_string"));

    auth::AuthorizeRequest authorize_request;
    std::promise<auth::AuthorizeResponse> request;

    client_->Authorize(kResponseToken, authorize_request,
                       [&](const auth::AuthorizeResponse& response) {
                         request.set_value(response);
                       });

    auto future = request.get_future();
    auto response = future.get();

    EXPECT_FALSE(response.IsSuccessful());
    auto error = response.GetError();
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::Unknown);

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
  {
    SCOPED_TRACE("Failed invalid request");

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                                     kAuthorizeResponseErrorInvalidRequest));

    auth::AuthorizeRequest authorize_request;
    std::promise<auth::AuthorizeResponse> request;

    client_->Authorize(kResponseToken, authorize_request,
                       [&](const auth::AuthorizeResponse& response) {
                         request.set_value(response);
                       });

    auto future = request.get_future();
    auto response = future.get();

    auto error = response.GetError();
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::Unknown);
    EXPECT_EQ(error.GetMessage(),
              "Error code: 400002 (Received invalid request. Invalid Json: "
              "Unexpected character ('[' (code 91)): was expecting "
              "double-quote to start field name\n at [Source: "
              "(akka.util.ByteIterator$ByteArrayIterator$$anon$1); line: 1, "
              "column: 3])");

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
}
