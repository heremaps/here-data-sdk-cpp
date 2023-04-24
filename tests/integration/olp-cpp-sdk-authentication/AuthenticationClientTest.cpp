/*
 * Copyright (C) 2019-2022 HERE Europe B.V.
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

#include <thread>

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
namespace http = olp::http;
using testing::_;

namespace {
constexpr auto kTimestampUrl = "https://authentication.server.url/timestamp";
constexpr auto kIntrospectUrl = "https://authentication.server.url/app/me";
constexpr auto kSignUpUrl = "https://authentication.server.url/user";
constexpr auto kGetMyAccountUrl = "https://authentication.server.url/user/me";
constexpr auto kTokenEndpointUrl = "https://authentication.server.url";
constexpr auto kTermsUrl = "https://authentication.server.url/terms";

constexpr unsigned int kExpiryTime = 3600;
constexpr unsigned int kMaxExpiryTime = kExpiryTime + 30;
constexpr unsigned int kMinExpiryTime = kExpiryTime - 10;
constexpr unsigned int kExpectedRetryCount = 3;

constexpr auto kMaxRetryAttempts = 5;
constexpr auto kRetryTimeout = 10;
constexpr auto kMinTimeout = 1;

// HTTP errors
constexpr auto kErrorOk = "OK";
constexpr auto kErrorSignupCreated = "Created";
constexpr auto kErrorServiceUnavailable = "Service unavailable";

constexpr auto kErrorNoContent = "No Content";
constexpr auto kErrorFieldsMessage = "Received invalid data.";
constexpr auto kErrorPreconditionFailedMessage = "Precondition Failed";

constexpr auto kErrorBadRequestMessage = "Invalid JSON.";

constexpr auto kErrorUnauthorizedMessage =
    "Signature mismatch. Authorization signature or client credential is "
    "wrong.";

constexpr auto kErrorUserNotFound =
    "User for the given access token cannot be found.";

constexpr auto kErrorConflictMessage =
    "A password account with the specified email address already exists.";

constexpr auto kErrorTooManyRequestsMessage =
    "Request blocked because too many requests were made. Please wait for a "
    "while before making a new request.";

constexpr auto kErrorInternalServerMessage = "Missing Thing Encrypted Secret.";

constexpr auto kErrorIllegalLastName = "Illegal last name.";
constexpr auto kErrorBlacklistedPassword = "Black listed password.";
constexpr auto kDateHeader = "Fri, 29 May 2020 11:07:45 GMT";
constexpr auto kRequestAuth = "https://authentication.server.url/oauth2/token";
constexpr auto kLogout = "https://authentication.server.url/logout";
constexpr auto kDate = "date";

constexpr auto kErrorFieldsCode = 400200;
constexpr auto kErrorBadRequestCode = 400002;
constexpr auto kErrorIllegalLastNameCode = 400203;
constexpr auto kErrorBlacklistedPasswordCode = 400205;
constexpr auto kErrorTooManyRequestsCode = 429002;
constexpr auto kErrorUnauthorizedCode = 401300;
constexpr auto kErrorNotFoundCode = 404000;
constexpr auto kErrorConflictCode = 409100;
constexpr auto kErrorInternalServerCode = 500203;

constexpr auto kWaitTimeout = std::chrono::seconds(5);

NetworkCallback MockWrongTimestamp() {
  return [](http::NetworkRequest /*request*/, http::Network::Payload payload,
            http::Network::Callback callback,
            http::Network::HeaderCallback header_callback,
            http::Network::DataCallback data_callback) {
    http::RequestId request_id(5);
    if (payload) {
      *payload << kResponseWrongTimestamp;
    }
    callback(http::NetworkResponse()
                 .WithRequestId(request_id)
                 .WithStatus(http::HttpStatusCode::UNAUTHORIZED));
    if (data_callback) {
      auto raw = const_cast<char*>(kResponseWrongTimestamp.c_str());
      data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                    kResponseWrongTimestamp.size());
    }
    if (header_callback) {
      header_callback(kDate, kDateHeader);
    }

    return http::SendOutcome(request_id);
  };
}

void ExpectNoTimestampRequest(NetworkMock& network) {
  EXPECT_CALL(network, Send(IsGetRequest(kTimestampUrl), _, _, _, _)).Times(0);
}

void ExpectTimestampRequest(NetworkMock& network, int32_t times = 1) {
  EXPECT_CALL(network, Send(IsGetRequest(kTimestampUrl), _, _, _, _))
      .Times(times)
      .WillRepeatedly(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                         kResponseTime));
}

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
  void SetUp() override {
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

  void TearDown() override {}

  auth::AuthenticationClient::SignUpResponse SignUpUser(
      const std::string& email) {
    auth::AuthenticationCredentials credentials(key_, secret_);
    std::promise<auth::AuthenticationClient::SignUpResponse> request;
    auto request_future = request.get_future();
    auth::AuthenticationClient::SignUpProperties properties;
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

    const bool is_retryable = client::DefaultRetryCondition({http});

    // First is GetTimeFromServer(). Second is actual SignIn.
    // When the request is retryable, 3 more requests are fired.
    const auto expected_number_of_calls =
        1 + (is_retryable ? kExpectedRetryCount : 1);

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .Times(expected_number_of_calls)
        .WillRepeatedly([&](http::NetworkRequest /*request*/,
                            http::Network::Payload payload,
                            http::Network::Callback callback,
                            http::Network::HeaderCallback /*header_callback*/,
                            http::Network::DataCallback data_callback) {
          http::RequestId request_id(5);
          if (payload) {
            *payload << data;
          }
          callback(http::NetworkResponse()
                       .WithRequestId(request_id)
                       .WithStatus(http));
          if (data_callback) {
            auto raw = const_cast<char*>(data.c_str());
            data_callback(reinterpret_cast<uint8_t*>(raw), 0, data.size());
          }

          return http::SendOutcome(request_id);
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

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

 protected:
  std::shared_ptr<NetworkMock> network_;
  std::unique_ptr<auth::AuthenticationClient> client_;
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

  ExpectNoTimestampRequest(*network_);

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
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

  ExpectNoTimestampRequest(*network_);

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
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

  ExpectNoTimestampRequest(*network_);

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(MockWrongTimestamp())
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
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
  ExpectTimestampRequest(*network_);

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
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
      .WillRepeatedly(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                         kResponseTime));

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .Times(2)
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kResponseValidJson))
      .WillOnce(testing::WithArg<2>([&](http::Network::Callback callback) {
        http::RequestId request_id(6);
        callback(http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(-1)
                     .WithError(""));

        return http::SendOutcome(request_id);
      }));

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
      .WillOnce([&](http::NetworkRequest /*request*/,
                    http::Network::Payload payload,
                    http::Network::Callback callback,
                    http::Network::HeaderCallback /*header_callback*/,
                    http::Network::DataCallback data_callback) {
        http::RequestId request_id(5);
        if (payload) {
          *payload << kSignupHereUserResponse;
        }
        callback(http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(http::HttpStatusCode::CREATED)
                     .WithError(kErrorSignupCreated));
        if (data_callback) {
          auto raw = const_cast<char*>(kSignupHereUserResponse.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kSignupHereUserResponse.size());
        }

        return http::SendOutcome(request_id);
      });

  auto response = SignUpUser("email@example.com");

  EXPECT_TRUE(response.IsSuccessful());
  const auto& result = response.GetResult();
  EXPECT_EQ(http::HttpStatusCode::CREATED, result.GetStatus());
  EXPECT_EQ(kErrorSignupCreated, result.GetErrorResponse().message);
  EXPECT_FALSE(result.GetUserIdentifier().empty());

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, SignInUserDataFirstTime) {
  ExpectTimestampRequest(*network_);

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(http::HttpStatusCode::PRECONDITION_FAILED)
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
  EXPECT_EQ(http::HttpStatusCode::PRECONDITION_FAILED,
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
  ExpectTimestampRequest(*network_);

  EXPECT_CALL(*network_, Send(IsPostRequest(kTermsUrl), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::NO_CONTENT)
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
  EXPECT_EQ(http::HttpStatusCode::NO_CONTENT, response.GetResult().GetStatus());
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
  ExpectTimestampRequest(*network_);

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(http::HttpStatusCode::OK).WithError(kErrorOk),
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
  EXPECT_EQ(http::HttpStatusCode::OK, response.GetResult().GetStatus());
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
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::NO_CONTENT)
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
  EXPECT_EQ(http::HttpStatusCode::NO_CONTENT, result.GetStatus());
  EXPECT_EQ(kErrorNoContent, result.GetErrorResponse().message);

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, SignInFederated) {
  ExpectTimestampRequest(*network_);

  std::string body =
      R"({ "grantType": "xyz", "token": "test_token", "realm": "my_realm" })";

  EXPECT_CALL(*network_,
              Send(testing::AllOf(HeadersContainAuthorization(), BodyEq(body),
                                  IsPostRequest(kRequestAuth)),
                   _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(http::HttpStatusCode::OK).WithError(kErrorOk),
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
  EXPECT_EQ(http::HttpStatusCode::OK, response.GetResult().GetStatus());
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
  ExpectTimestampRequest(*network_);

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(http::HttpStatusCode::OK).WithError(kErrorOk),
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
  EXPECT_EQ(http::HttpStatusCode::OK, response.GetResult().GetStatus());
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
  ExpectTimestampRequest(*network_);

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(http::HttpStatusCode::OK).WithError(kErrorOk),
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
  EXPECT_EQ(http::HttpStatusCode::OK, response.GetResult().GetStatus());
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

TEST_F(AuthenticationClientTest, SignInApple) {
  auth::AppleSignInProperties properties;

  {
    SCOPED_TRACE("Failed request with error code");

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .WillOnce(testing::WithoutArgs([]() {
          return http::SendOutcome(http::ErrorCode::AUTHENTICATION_ERROR);
        }));

    std::promise<auth::AuthenticationClient::SignInUserResponse> request;

    client_->SignInApple(
        properties,
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          request.set_value(response);
        });

    auto request_future = request.get_future();
    auto response = request_future.get();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::SERVICE_UNAVAILABLE,
              response.GetResult().GetStatus());
    EXPECT_EQ(kErrorServiceUnavailable,
              response.GetResult().GetErrorResponse().message);

    ::testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Successful response with HTTP error");

    auto get_retry_max_attempts_count = [] {
      client::RetrySettings retry_settings;
      return retry_settings.max_attempts;
    };

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .Times(get_retry_max_attempts_count() + 1 /* first request */)
        .WillRepeatedly(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS)
                .WithError(kErrorTooManyRequestsMessage),
            kResponseTooManyRequests));

    std::promise<auth::AuthenticationClient::SignInUserResponse> request;

    client_->SignInApple(
        properties,
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          request.set_value(response);
        });

    auto request_future = request.get_future();
    auto response = request_future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::TOO_MANY_REQUESTS,
              response.GetResult().GetStatus());
    EXPECT_EQ(kErrorTooManyRequestsMessage,
              response.GetResult().GetErrorResponse().message);

    ::testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Successful response");

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::OK).WithError(kErrorOk),
            kAppleSignInResponse));

    std::time_t now = std::time(nullptr);
    std::promise<auth::AuthenticationClient::SignInUserResponse> request;

    client_->SignInApple(
        properties,
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          request.set_value(response);
        });

    auto request_future = request.get_future();
    auto response = request_future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::OK, response.GetResult().GetStatus());
    EXPECT_EQ(kErrorOk, response.GetResult().GetErrorResponse().message);
    EXPECT_EQ("apple_grant_token", response.GetResult().GetAccessToken());
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

  {
    SCOPED_TRACE("Cache check");

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .WillOnce(testing::WithoutArgs([]() -> http::SendOutcome {
          return http::SendOutcome(http::ErrorCode::CANCELLED_ERROR);
        }));

    std::promise<auth::AuthenticationClient::SignInUserResponse> request;

    client_->SignInApple(
        properties,
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          request.set_value(response);
        });

    auto request_future = request.get_future();
    auto response = request_future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::OK, response.GetResult().GetStatus());
    EXPECT_EQ(kErrorOk, response.GetResult().GetErrorResponse().message);
    EXPECT_EQ("apple_grant_token", response.GetResult().GetAccessToken());

    ::testing::Mock::VerifyAndClearExpectations(network_.get());
  }
}

TEST_F(AuthenticationClientTest, SignInRefreshData) {
  ExpectTimestampRequest(*network_);

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(http::HttpStatusCode::OK).WithError(kErrorOk),
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
  EXPECT_EQ(http::HttpStatusCode::OK, response.GetResult().GetStatus());
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
  ExpectTimestampRequest(*network_);
  {
    SCOPED_TRACE("SignInHereUser");

    EXPECT_CALL(*network_, Send(testing::AnyOf(IsPostRequest(kRequestAuth),
                                               IsPostRequest(kLogout),
                                               IsPostRequest(kSignUpUrl)),
                                _, _, _, _))
        .Times(3)
        .WillRepeatedly([&](http::NetworkRequest /*request*/,
                            http::Network::Payload payload,
                            http::Network::Callback callback,
                            http::Network::HeaderCallback /*header_callback*/,
                            http::Network::DataCallback data_callback) {
          http::RequestId request_id(5);
          if (payload) {
            *payload << kResponseErrorFields;
          }
          callback(http::NetworkResponse()
                       .WithRequestId(request_id)
                       .WithStatus(http::HttpStatusCode::BAD_REQUEST)
                       .WithError(kErrorFieldsMessage));
          if (data_callback) {
            auto raw = const_cast<char*>(kResponseErrorFields.c_str());
            data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                          kResponseErrorFields.size());
          }
          return http::SendOutcome(request_id);
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
    EXPECT_EQ(http::HttpStatusCode::BAD_REQUEST,
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
    EXPECT_EQ(http::HttpStatusCode::BAD_REQUEST,
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
    EXPECT_EQ(http::HttpStatusCode::BAD_REQUEST,
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

    ExecuteSigninRequest(http::HttpStatusCode::OK,
                         http::HttpStatusCode::SERVICE_UNAVAILABLE,
                         kErrorServiceUnavailable, kResponseInvalidJson);
  }

  {
    SCOPED_TRACE("No token");

    ExecuteSigninRequest(http::HttpStatusCode::OK,
                         http::HttpStatusCode::SERVICE_UNAVAILABLE,
                         kErrorServiceUnavailable, kResponseNoToken);
  }

  {
    SCOPED_TRACE("Token type missing");

    ExecuteSigninRequest(http::HttpStatusCode::OK,
                         http::HttpStatusCode::SERVICE_UNAVAILABLE,
                         kErrorServiceUnavailable, kResponseNoTokenType);
  }

  {
    SCOPED_TRACE("Missing expiry");

    ExecuteSigninRequest(http::HttpStatusCode::OK,
                         http::HttpStatusCode::SERVICE_UNAVAILABLE,
                         kErrorServiceUnavailable, kResponseNoExpiry);
  }

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, TestHttpRequestErrorCodes) {
  auto status = http::HttpStatusCode::OK;

  {
    SCOPED_TRACE("ACCEPTED");

    status = http::HttpStatusCode::ACCEPTED;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("CREATED");

    status = http::HttpStatusCode::CREATED;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status),
                         kResponseCreated);
  }

  {
    SCOPED_TRACE("NON_AUTHORITATIVE_INFORMATION");

    status = http::HttpStatusCode::NON_AUTHORITATIVE_INFORMATION;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("NO_CONTENT");

    status = http::HttpStatusCode::NO_CONTENT;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status),
                         kResponseNoContent);
  }

  {
    SCOPED_TRACE("RESET_CONTENT");

    status = http::HttpStatusCode::RESET_CONTENT;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("MULTIPLE_CHOICES");

    status = http::HttpStatusCode::PARTIAL_CONTENT;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("MULTIPLE_CHOICES");

    status = http::HttpStatusCode::MULTIPLE_CHOICES;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("MOVED_PERMANENTLY");

    status = http::HttpStatusCode::MOVED_PERMANENTLY;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("FOUND");

    status = http::HttpStatusCode::FOUND;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("SEE_OTHER");

    status = http::HttpStatusCode::SEE_OTHER;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("NOT_MODIFIED");

    status = http::HttpStatusCode::NOT_MODIFIED;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("USE_PROXY");

    status = http::HttpStatusCode::USE_PROXY;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("BAD_REQUEST");

    status = http::HttpStatusCode::BAD_REQUEST;
    ExecuteSigninRequest(status, status, kErrorBadRequestMessage,
                         kResponseBadRequest, kErrorBadRequestCode);
  }

  {
    SCOPED_TRACE("UNAUTHORIZED");

    status = http::HttpStatusCode::UNAUTHORIZED;
    ExecuteSigninRequest(status, status, kErrorUnauthorizedMessage,
                         kResponseUnauthorized, kErrorUnauthorizedCode);
  }

  {
    SCOPED_TRACE("PAYMENT_REQUIRED");

    status = http::HttpStatusCode::PAYMENT_REQUIRED;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("NOT_FOUND");

    status = http::HttpStatusCode::NOT_FOUND;
    ExecuteSigninRequest(status, status, kErrorUserNotFound, kResponseNotFound,
                         kErrorNotFoundCode);
  }

  {
    SCOPED_TRACE("METHOD_NOT_ALLOWED");

    status = http::HttpStatusCode::METHOD_NOT_ALLOWED;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("FORBIDDEN");

    status = http::HttpStatusCode::FORBIDDEN;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("NOT_ACCEPTABLE");

    status = http::HttpStatusCode::NOT_ACCEPTABLE;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("PROXY_AUTHENTICATION_REQUIRED");

    status = http::HttpStatusCode::PROXY_AUTHENTICATION_REQUIRED;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("REQUEST_TIMEOUT");

    status = http::HttpStatusCode::REQUEST_TIMEOUT;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("CONFLICT");

    status = http::HttpStatusCode::CONFLICT;
    ExecuteSigninRequest(status, status, kErrorConflictMessage,
                         kResponseConflict, kErrorConflictCode);
  }

  {
    SCOPED_TRACE("GONE");

    status = http::HttpStatusCode::GONE;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("LENGTH_REQUIRED");

    status = http::HttpStatusCode::LENGTH_REQUIRED;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("PRECONDITION_FAILED");

    status = http::HttpStatusCode::PRECONDITION_FAILED;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status),
                         kResponsePreconditionFailed);
  }

  {
    SCOPED_TRACE("REQUEST_ENTITY_TOO_LARGE");

    status = http::HttpStatusCode::REQUEST_ENTITY_TOO_LARGE;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("REQUEST_URI_TOO_LONG");

    status = http::HttpStatusCode::REQUEST_URI_TOO_LONG;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("UNSUPPORTED_MEDIA_TYPE");

    status = http::HttpStatusCode::UNSUPPORTED_MEDIA_TYPE;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("TOO_MANY_REQUESTS");

    status = http::HttpStatusCode::TOO_MANY_REQUESTS;
    ExecuteSigninRequest(status, status, kErrorTooManyRequestsMessage,
                         kResponseTooManyRequests, kErrorTooManyRequestsCode);
  }

  {
    SCOPED_TRACE("INTERNAL_SERVER_ERROR");

    status = http::HttpStatusCode::INTERNAL_SERVER_ERROR;
    ExecuteSigninRequest(status, status, kErrorInternalServerMessage,
                         kResponseInternalServerError,
                         kErrorInternalServerCode);
  }

  {
    SCOPED_TRACE("NOT_IMPLEMENTED");

    status = http::HttpStatusCode::NOT_IMPLEMENTED;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("BAD_GATEWAY");

    status = http::HttpStatusCode::BAD_GATEWAY;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("SERVICE_UNAVAILABLE");

    status = http::HttpStatusCode::SERVICE_UNAVAILABLE;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("GATEWAY_TIMEOUT");

    status = http::HttpStatusCode::GATEWAY_TIMEOUT;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("VERSION_NOT_SUPPORTED");

    status = http::HttpStatusCode::VERSION_NOT_SUPPORTED;
    ExecuteSigninRequest(status, status, http::HttpErrorToString(status));
  }

  {
    SCOPED_TRACE("Custom positive status");

    auto custom_error = 100000;
    ExecuteSigninRequest(custom_error, custom_error,
                         http::HttpErrorToString(custom_error));
  }

  {
    SCOPED_TRACE("Custom negative status");

    auto custom_error = -100000;
    ExecuteSigninRequest(custom_error, custom_error,
                         http::HttpErrorToString(custom_error));
  }

  ::testing::Mock::VerifyAndClearExpectations(network_.get());
}

TEST_F(AuthenticationClientTest, IntrospectApp) {
  {
    SCOPED_TRACE("Successful request");

    EXPECT_CALL(*network_, Send(IsGetRequest(kIntrospectUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
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
        .WillOnce(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::UNAUTHORIZED),
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
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     "Invalid response"));

    std::promise<auth::IntrospectAppResponse> request;

    client_->IntrospectApp(kResponseToken,
                           [&](const auth::IntrospectAppResponse& response) {
                             request.set_value(response);
                           });

    auto future = request.get_future();
    auto response = future.get();
    auto error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::Unknown);

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
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
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
    ASSERT_EQ(result.GetDecision(), auth::DecisionType::kAllow);

    auto it = result.GetActionResults().begin();
    ASSERT_EQ(it->GetDecision(), auth::DecisionType::kAllow);
    ASSERT_EQ(it->GetPermissions().front().GetAction(), "read");
    ASSERT_EQ(it->GetPermissions().front().GetResource(), "some_resource");
    ASSERT_EQ(it->GetPermissions().front().GetDecision(),
              auth::DecisionType::kAllow);

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
  {
    SCOPED_TRACE("Failed request");

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
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
        .WillOnce(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::UNAUTHORIZED),
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
            testing::WithArg<2>([&](http::Network::Callback callback) {
              http::RequestId request_id(3);

              callback(
                  http::NetworkResponse()
                      .WithRequestId(request_id)
                      .WithStatus(http::HttpStatusCode::SERVICE_UNAVAILABLE));
              return http::SendOutcome(request_id);
            }));

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
    SCOPED_TRACE("Failed corrupted response");

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
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
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
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

TEST_F(AuthenticationClientTest, GetMyAccount) {
  {
    SCOPED_TRACE("Successful request");

    EXPECT_CALL(*network_, Send(IsGetRequest(kGetMyAccountUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kGetMyAccountResponse));

    std::promise<auth::UserAccountInfoResponse> request;

    client_->GetMyAccount(kResponseToken,
                          [&](const auth::UserAccountInfoResponse& response) {
                            request.set_value(response);
                          });

    auto future = request.get_future();
    auto response = future.get();
    auto result = response.GetResult();
    auto error = response.GetError();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_FALSE(result.GetUserId().empty());
    EXPECT_FALSE(result.GetRealm().empty());
    EXPECT_FALSE(result.GetFacebookId().empty());
    EXPECT_FALSE(result.GetFirstname().empty());
    EXPECT_FALSE(result.GetLastname().empty());
    EXPECT_FALSE(result.GetEmail().empty());
    EXPECT_FALSE(result.GetRecoveryEmail().empty());
    EXPECT_FALSE(result.GetDob().empty());
    EXPECT_FALSE(result.GetCountryCode().empty());
    EXPECT_FALSE(result.GetLanguage().empty());
    EXPECT_TRUE(result.GetEmailVerified());
    EXPECT_FALSE(result.GetPhoneNumber().empty());
    EXPECT_TRUE(result.GetPhoneNumberVerified());
    EXPECT_TRUE(result.GetMarketingEnabled());
    EXPECT_TRUE(result.GetCreatedTime() > 0);
    EXPECT_TRUE(result.GetUpdatedTime() > 0);
    EXPECT_FALSE(result.GetState().empty());
    EXPECT_FALSE(result.GetHrn().empty());
    EXPECT_FALSE(result.GetAccountType().empty());

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
  {
    SCOPED_TRACE("Invalid access token");

    EXPECT_CALL(*network_, Send(IsGetRequest(kGetMyAccountUrl), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::UNAUTHORIZED),
                               kInvalidAccessTokenResponse));

    std::promise<auth::UserAccountInfoResponse> request;

    client_->GetMyAccount(kResponseToken,
                          [&](const auth::UserAccountInfoResponse& response) {
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

    EXPECT_CALL(*network_, Send(IsGetRequest(kGetMyAccountUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     "Invalid response"));

    std::promise<auth::UserAccountInfoResponse> request;

    client_->GetMyAccount(kResponseToken,
                          [&](const auth::UserAccountInfoResponse& response) {
                            request.set_value(response);
                          });

    auto future = request.get_future();
    auto response = future.get();
    auto error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::Unknown);

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
}

TEST_F(AuthenticationClientTest, UniqueNonce) {
  auth::AuthenticationCredentials credentials(key_, secret_);

  std::set<std::string> nonces;

  auto extract_nonce = [&](const http::Headers& headers) {
    auto auth_it =
        std::find_if(headers.begin(), headers.end(),
                     [](const std::pair<std::string, std::string>& header) {
                       return header.first == http::kAuthorizationHeader;
                     });
    if (auth_it == headers.end()) {
      return;
    }

    const auto& params = auth_it->second;
    const auto prefix = "oauth_nonce=\"";
    auto nonce_begin = params.find(prefix);
    if (nonce_begin == std::string::npos) {
      return;
    }
    nonce_begin += strlen(prefix);
    auto nonce_end = params.find("\"", nonce_begin);
    if (nonce_end == std::string::npos) {
      return;
    }
    auto nonce = params.substr(nonce_begin, nonce_end - nonce_begin);
    nonces.insert(nonce);
  };

  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(3)
      .WillRepeatedly(testing::WithArgs<0, 2>(
          [&](http::NetworkRequest request, http::Network::Callback callback) {
            http::RequestId request_id(5);
            extract_nonce(request.GetHeaders());
            callback(http::NetworkResponse()
                         .WithRequestId(request_id)
                         .WithStatus(http::HttpStatusCode::TOO_MANY_REQUESTS));
            return http::SendOutcome(request_id);
          }));

  ExpectTimestampRequest(*network_);

  std::promise<void> barrier;

  client_->SignInClient(
      credentials, {},
      [&](const auth::AuthenticationClient::SignInClientResponse&) {
        barrier.set_value();
      });

  auto future = barrier.get_future();

  future.wait_for(kWaitTimeout);

  testing::Mock::VerifyAndClearExpectations(network_.get());

  EXPECT_EQ(nonces.size(), kExpectedRetryCount);
}

TEST_F(AuthenticationClientTest, RetrySettings) {
  auth::AuthenticationSettings settings;
  settings.network_request_handler = network_;
  settings.token_endpoint_url = kTokenEndpointUrl;
  settings.task_scheduler = task_scheduler_;
  settings.use_system_time = true;
  settings.retry_settings.max_attempts = kMaxRetryAttempts;
  settings.retry_settings.timeout = kRetryTimeout;

  const auto retry_predicate = testing::Property(
      &http::NetworkRequest::GetSettings,
      testing::AllOf(
          testing::Property(
              &http::NetworkSettings::GetConnectionTimeoutDuration,
              std::chrono::seconds(kRetryTimeout)),
          testing::Property(&http::NetworkSettings::GetTransferTimeoutDuration,
                            std::chrono::seconds(kRetryTimeout))));

  ON_CALL(*network_, Send(retry_predicate, _, _, _, _))
      .WillByDefault(ReturnHttpResponse(
          GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS)
              .WithError(kErrorTooManyRequestsMessage),
          kResponseTooManyRequests));

  const auth::AuthenticationCredentials credentials(key_, secret_);
  client_ = std::make_unique<auth::AuthenticationClient>(std::move(settings));

  {
    SCOPED_TRACE("SignInClient");

    EXPECT_CALL(*network_, Send(retry_predicate, _, _, _, _))
        .Times(kMaxRetryAttempts);

    std::promise<auth::AuthenticationClient::SignInClientResponse>
        response_promise;
    client_->SignInClient(
        credentials, auth::AuthenticationClient::SignInProperties{},
        [&](const auth::AuthenticationClient::SignInClientResponse& value) {
          response_promise.set_value(value);
        });

    const auto response = response_promise.get_future().get();
    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::TOO_MANY_REQUESTS,
              response.GetResult().GetStatus());

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("SignInHereUser");

    EXPECT_CALL(*network_, Send(retry_predicate, _, _, _, _))
        .Times(kMaxRetryAttempts);

    std::promise<auth::AuthenticationClient::SignInUserResponse>
        response_promise;
    client_->SignInHereUser(
        credentials, auth::AuthenticationClient::UserProperties{},
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          response_promise.set_value(response);
        });

    const auto response = response_promise.get_future().get();
    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::TOO_MANY_REQUESTS,
              response.GetResult().GetStatus());

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("SignInFederated");

    EXPECT_CALL(*network_, Send(retry_predicate, _, _, _, _))
        .Times(kMaxRetryAttempts);

    std::promise<auth::AuthenticationClient::SignInUserResponse>
        response_promise;
    client_->SignInFederated(
        credentials,
        R"({ "grantType": "xyz", "token": "test_token", "realm": "my_realm" })",
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          response_promise.set_value(response);
        });

    const auto response = response_promise.get_future().get();
    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::TOO_MANY_REQUESTS,
              response.GetResult().GetStatus());

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("SignInApple");

    EXPECT_CALL(*network_, Send(retry_predicate, _, _, _, _))
        .Times(kMaxRetryAttempts + 1 /* first request */);

    std::promise<auth::AuthenticationClient::SignInUserResponse>
        response_promise;
    client_->SignInApple(
        auth::AppleSignInProperties{},
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          response_promise.set_value(response);
        });

    const auto response = response_promise.get_future().get();
    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::TOO_MANY_REQUESTS,
              response.GetResult().GetStatus());

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("SignInRefresh");

    EXPECT_CALL(*network_, Send(retry_predicate, _, _, _, _))
        .Times(kMaxRetryAttempts);

    std::promise<auth::AuthenticationClient::SignInUserResponse>
        response_promise;
    client_->SignInRefresh(
        credentials, auth::AuthenticationClient::RefreshProperties{},
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          response_promise.set_value(response);
        });

    const auto response = response_promise.get_future().get();
    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::TOO_MANY_REQUESTS,
              response.GetResult().GetStatus());

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("AcceptTerms");

    EXPECT_CALL(*network_, Send(retry_predicate, _, _, _, _))
        .Times(kMaxRetryAttempts);

    std::promise<auth::AuthenticationClient::SignInUserResponse>
        response_promise;
    client_->AcceptTerms(
        credentials, "reacceptance_token",
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          response_promise.set_value(response);
        });

    const auto response = response_promise.get_future().get();
    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::TOO_MANY_REQUESTS,
              response.GetResult().GetStatus());

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("SignUpHereUser");

    EXPECT_CALL(*network_, Send(retry_predicate, _, _, _, _)).Times(1);

    std::promise<auth::AuthenticationClient::SignUpResponse> response_promise;
    client_->SignUpHereUser(
        credentials, auth::AuthenticationClient::SignUpProperties{},
        [&](const auth::AuthenticationClient::SignUpResponse& response) {
          response_promise.set_value(response);
        });

    const auto response = response_promise.get_future().get();
    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::TOO_MANY_REQUESTS,
              response.GetResult().GetStatus());

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("SignOut");

    EXPECT_CALL(*network_, Send(retry_predicate, _, _, _, _)).Times(1);

    std::promise<auth::AuthenticationClient::SignOutUserResponse>
        response_promise;
    client_->SignOut(
        credentials, kResponseToken,
        [&](const auth::AuthenticationClient::SignOutUserResponse& response) {
          response_promise.set_value(response);
        });

    const auto response = response_promise.get_future().get();
    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::TOO_MANY_REQUESTS,
              response.GetResult().GetStatus());

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("IntrospectApp");

    EXPECT_CALL(*network_, Send(retry_predicate, _, _, _, _))
        .Times(kMaxRetryAttempts + 1 /* first request */);

    std::promise<auth::IntrospectAppResponse> response_promise;
    client_->IntrospectApp(kResponseToken,
                           [&](const auth::IntrospectAppResponse& response) {
                             response_promise.set_value(response);
                           });

    const auto response = response_promise.get_future().get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::TOO_MANY_REQUESTS,
              response.GetError().GetHttpStatusCode());

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Authorize");

    EXPECT_CALL(*network_, Send(retry_predicate, _, _, _, _))
        .Times(kMaxRetryAttempts + 1 /* first request */);

    std::promise<auth::AuthorizeResponse> response_promise;
    client_->Authorize(kResponseToken, auth::AuthorizeRequest{},
                       [&](const auth::AuthorizeResponse& response) {
                         response_promise.set_value(response);
                       });

    const auto response = response_promise.get_future().get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::TOO_MANY_REQUESTS,
              response.GetError().GetHttpStatusCode());

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("GetMyAccount");

    EXPECT_CALL(*network_, Send(retry_predicate, _, _, _, _))
        .Times(kMaxRetryAttempts + 1 /* first request */);

    std::promise<auth::UserAccountInfoResponse> response_promise;
    client_->GetMyAccount(kResponseToken,
                          [&](const auth::UserAccountInfoResponse& response) {
                            response_promise.set_value(response);
                          });

    const auto response = response_promise.get_future().get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(http::HttpStatusCode::TOO_MANY_REQUESTS,
              response.GetError().GetHttpStatusCode());

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
}

TEST_F(AuthenticationClientTest, Timeout) {
  auth::AuthenticationSettings settings;
  settings.network_request_handler = network_;
  settings.token_endpoint_url = kTokenEndpointUrl;
  settings.task_scheduler = task_scheduler_;
  settings.use_system_time = true;
  settings.retry_settings.timeout = kMinTimeout;

  const auth::AuthenticationCredentials credentials(key_, secret_);
  client_ = std::make_unique<auth::AuthenticationClient>(std::move(settings));

  http::RequestId request_id = 0u;
  std::future<void> async_finish_future;

  ON_CALL(*network_, Send(_, _, _, _, _))
      .WillByDefault(testing::WithArg<2>([&](http::Network::Callback callback) {
        async_finish_future = std::async(std::launch::async, [&, callback]() {
          // Oversleep the timeout period.
          std::this_thread::sleep_for(std::chrono::seconds(kMinTimeout * 2));

          callback(http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(request_id));
        });

        return http::SendOutcome(request_id);
      }));

  {
    SCOPED_TRACE("SignInClient");

    async_finish_future = std::future<void>{};
    request_id++;

    EXPECT_CALL(*network_, Send(_, _, _, _, _)).Times(1);
    EXPECT_CALL(*network_, Cancel(request_id)).Times(1);

    std::promise<auth::AuthenticationClient::SignInClientResponse>
        response_promise;
    client_->SignInClient(
        credentials, auth::AuthenticationClient::SignInProperties{},
        [&](const auth::AuthenticationClient::SignInClientResponse& value) {
          response_promise.set_value(value);
        });

    auto response_future = response_promise.get_future();
    ASSERT_EQ(response_future.wait_for(kWaitTimeout),
              std::future_status::ready);

    const auto response = response_future.get();
    ASSERT_FALSE(response);
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              static_cast<int>(http::ErrorCode::TIMEOUT_ERROR));

    ASSERT_TRUE(async_finish_future.valid());
    ASSERT_EQ(async_finish_future.wait_for(kWaitTimeout),
              std::future_status::ready);

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("SignInHereUser");

    async_finish_future = std::future<void>{};
    request_id++;

    EXPECT_CALL(*network_, Send(_, _, _, _, _)).Times(1);
    EXPECT_CALL(*network_, Cancel(request_id)).Times(1);

    std::promise<auth::AuthenticationClient::SignInUserResponse>
        response_promise;
    client_->SignInHereUser(
        credentials, auth::AuthenticationClient::UserProperties{},
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          response_promise.set_value(response);
        });

    auto response_future = response_promise.get_future();
    ASSERT_EQ(response_future.wait_for(kWaitTimeout),
              std::future_status::ready);

    const auto response = response_future.get();
    ASSERT_FALSE(response);
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              static_cast<int>(http::ErrorCode::TIMEOUT_ERROR));

    ASSERT_TRUE(async_finish_future.valid());
    ASSERT_EQ(async_finish_future.wait_for(kWaitTimeout),
              std::future_status::ready);

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("SignInFederated");

    async_finish_future = std::future<void>{};
    request_id++;

    EXPECT_CALL(*network_, Send(_, _, _, _, _)).Times(1);
    EXPECT_CALL(*network_, Cancel(request_id)).Times(1);

    std::promise<auth::AuthenticationClient::SignInUserResponse>
        response_promise;
    client_->SignInFederated(
        credentials,
        R"({ "grantType": "xyz", "token": "test_token", "realm": "my_realm" })",
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          response_promise.set_value(response);
        });

    auto response_future = response_promise.get_future();
    ASSERT_EQ(response_future.wait_for(kWaitTimeout),
              std::future_status::ready);

    const auto response = response_future.get();
    ASSERT_FALSE(response);
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              static_cast<int>(http::ErrorCode::TIMEOUT_ERROR));

    ASSERT_TRUE(async_finish_future.valid());
    ASSERT_EQ(async_finish_future.wait_for(kWaitTimeout),
              std::future_status::ready);

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("SignInApple");

    async_finish_future = std::future<void>{};
    request_id++;

    EXPECT_CALL(*network_, Send(_, _, _, _, _)).Times(1);
    EXPECT_CALL(*network_, Cancel(request_id)).Times(1);

    std::promise<auth::AuthenticationClient::SignInUserResponse>
        response_promise;
    client_->SignInApple(
        auth::AppleSignInProperties{},
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          response_promise.set_value(response);
        });

    auto response_future = response_promise.get_future();
    ASSERT_EQ(response_future.wait_for(kWaitTimeout),
              std::future_status::ready);

    const auto response = response_future.get();
    ASSERT_FALSE(response);
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              static_cast<int>(http::ErrorCode::TIMEOUT_ERROR));

    ASSERT_TRUE(async_finish_future.valid());
    ASSERT_EQ(async_finish_future.wait_for(kWaitTimeout),
              std::future_status::ready);

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("SignInRefresh");

    async_finish_future = std::future<void>{};
    request_id++;

    EXPECT_CALL(*network_, Send(_, _, _, _, _)).Times(1);
    EXPECT_CALL(*network_, Cancel(request_id)).Times(1);

    std::promise<auth::AuthenticationClient::SignInUserResponse>
        response_promise;
    client_->SignInRefresh(
        credentials, auth::AuthenticationClient::RefreshProperties{},
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          response_promise.set_value(response);
        });

    auto response_future = response_promise.get_future();
    ASSERT_EQ(response_future.wait_for(kWaitTimeout),
              std::future_status::ready);

    const auto response = response_future.get();
    ASSERT_FALSE(response);
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              static_cast<int>(http::ErrorCode::TIMEOUT_ERROR));

    ASSERT_TRUE(async_finish_future.valid());
    ASSERT_EQ(async_finish_future.wait_for(kWaitTimeout),
              std::future_status::ready);

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("AcceptTerms");

    async_finish_future = std::future<void>{};
    request_id++;

    EXPECT_CALL(*network_, Send(_, _, _, _, _)).Times(1);
    EXPECT_CALL(*network_, Cancel(request_id)).Times(1);

    std::promise<auth::AuthenticationClient::SignInUserResponse>
        response_promise;
    client_->AcceptTerms(
        credentials, "reacceptance_token",
        [&](const auth::AuthenticationClient::SignInUserResponse& response) {
          response_promise.set_value(response);
        });

    auto response_future = response_promise.get_future();
    ASSERT_EQ(response_future.wait_for(kWaitTimeout),
              std::future_status::ready);

    const auto response = response_future.get();
    ASSERT_FALSE(response);
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              static_cast<int>(http::ErrorCode::TIMEOUT_ERROR));

    ASSERT_TRUE(async_finish_future.valid());
    ASSERT_EQ(async_finish_future.wait_for(kWaitTimeout),
              std::future_status::ready);

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
}

TEST_F(AuthenticationClientTest, SignInWithDevice) {
  auth::AuthenticationSettings settings;
  settings.network_request_handler = network_;
  settings.token_endpoint_url = kTokenEndpointUrl;
  settings.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();

  std::string body =
      R"({"grantType":"client_credentials","deviceId":"my-device-1"})";

  EXPECT_CALL(*network_,
              Send(testing::AllOf(HeadersContainAuthorization(), BodyEq(body),
                                  IsPostRequest(kRequestAuth)),
                   _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(http::HttpStatusCode::OK).WithError(kErrorOk),
          std::string()));

  std::promise<void> request;
  auth::AuthenticationClient::SignInClientResponse response;
  auth::AuthenticationClient::SignInProperties properties;
  properties.device_id = "my-device-1";

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
