/*
 * Copyright (C) 2021 HERE Europe B.V.
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

#include <olp/authentication/TokenEndpoint.h>

#include <future>
#include <memory>

#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/NetworkMock.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/NetworkConstants.h>
#include <olp/core/http/NetworkUtils.h>
#include <olp/core/porting/make_unique.h>
#include "AuthenticationMockedResponses.h"

namespace auth = olp::authentication;
namespace client = olp::client;
namespace http = olp::http;
using testing::_;

namespace {
constexpr auto kKey = "key";
constexpr auto kSecret = "secret";

constexpr auto kTimestampUrl = "https://authentication.server.url/timestamp";
constexpr auto kTokenEndpointUrl = "https://authentication.server.url";

constexpr unsigned int kExpiryTime = 3600;
constexpr unsigned int kMaxExpiryTime = kExpiryTime + 30;
constexpr unsigned int kMinExpiryTime = kExpiryTime - 10;
constexpr unsigned int kExpectedRetryCount = 3;

constexpr auto kWaitTimeout = std::chrono::seconds(3);
constexpr auto kMaxRetryAttempts = 5;
constexpr auto kRetryTimeout = 10;
constexpr auto kMinTimeout = 1;

constexpr http::RequestId kRequestId = 5;

constexpr auto kErrorServiceUnavailable = "Service Unavailable";
constexpr auto kErrorTimeOut = "Network request timed out.";
constexpr auto kErrorCancelled = "Cancelled";
constexpr auto kErrorBadRequestFullMessage =
    R"JSON({"errorCode":400002,"message":"Invalid JSON."})JSON";
constexpr auto kErrorUnauthorizedFullMessage =
    R"JSON({"errorCode":401300,"message":"Signature mismatch. Authorization signature or client credential is wrong."})JSON";
constexpr auto kErrorUserNotFoundFullMessage =
    R"JSON({"errorCode":404000,"message":"User for the given access token cannot be found."})JSON";
constexpr auto kErrorConflictFullMessage =
    R"JSON({"errorCode":409100,"message":"A password account with the specified email address already exists."})JSON";
constexpr auto kErrorTooManyRequestsFullMessage =
    R"JSON({"errorCode":429002,"message":"Request blocked because too many requests were made. Please wait for a while before making a new request."})JSON";
constexpr auto kErrorInternalServerFullMessage =
    R"JSON({"errorCode":500203,"message":"Missing Thing Encrypted Secret."})JSON";
constexpr auto kDateHeader = "Fri, 29 May 2020 11:07:45 GMT";
constexpr auto kRequestAuth = "https://authentication.server.url/oauth2/token";
constexpr auto kDate = "date";

NetworkCallback MockWrongTimestamp() {
  return [](http::NetworkRequest /*request*/, http::Network::Payload payload,
            http::Network::Callback callback,
            http::Network::HeaderCallback header_callback,
            http::Network::DataCallback data_callback) {
    if (payload) {
      *payload << kResponseWrongTimestamp;
    }
    callback(http::NetworkResponse()
                 .WithRequestId(kRequestId)
                 .WithStatus(http::HttpStatusCode::UNAUTHORIZED));
    if (data_callback) {
      auto raw = const_cast<char*>(kResponseWrongTimestamp.c_str());
      data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                    kResponseWrongTimestamp.size());
    }
    if (header_callback) {
      header_callback(kDate, kDateHeader);
    }

    return http::SendOutcome(kRequestId);
  };
}

void ExpectNoTimestampRequest(NetworkMock& network) {
  EXPECT_CALL(network, Send(IsGetRequest(kTimestampUrl), _, _, _, _)).Times(0);
}

void ExpectTimestampRequest(NetworkMock& network) {
  EXPECT_CALL(network, Send(IsGetRequest(kTimestampUrl), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kResponseTime));
}
}  // namespace

PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

class TokenEndpointTest : public testing::Test {
 public:
  void SetUp() override {
    network_ = std::make_shared<NetworkMock>();
    task_scheduler_ =
        client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(2u);

    settings_.network_request_handler = network_;
    settings_.task_scheduler = task_scheduler_;
    settings_.token_endpoint_url = kTokenEndpointUrl;

    PrepareEndpoint();
  }

  void TearDown() override {
    endpoint_.reset();
    task_scheduler_.reset();
    network_.reset();
  }

  void PrepareEndpoint(bool use_system_time = true) {
    settings_.use_system_time = use_system_time;
    endpoint_ = std::make_unique<auth::TokenEndpoint>(settings_);
  }

  void ExecuteRequestTokenWithError(int http_response,
                                    const std::string& error_message,
                                    const std::string& response_data = "") {
    const bool is_retryable = client::DefaultRetryCondition({http_response});

    const auto expected_calls_count =
        is_retryable ? settings_.retry_settings.max_attempts : 1;

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .Times(expected_calls_count)
        .WillRepeatedly(testing::WithArgs<1, 2, 4>(
            [&](http::Network::Payload payload,
                http::Network::Callback callback,
                http::Network::DataCallback data_callback) {
              if (payload) {
                *payload << response_data;
              }

              callback(http::NetworkResponse()
                           .WithRequestId(kRequestId)
                           .WithStatus(http_response));
              if (data_callback) {
                auto raw = const_cast<char*>(response_data.c_str());
                data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                              response_data.size());
              }

              return http::SendOutcome(kRequestId);
            }));

    client::CancellationContext context;
    const auto token_response = endpoint_->RequestToken(context);

    ASSERT_FALSE(token_response);
    EXPECT_EQ(token_response.GetError().GetHttpStatusCode(), http_response);
    EXPECT_EQ(token_response.GetError().GetMessage(), error_message);

    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

 protected:
  const auth::AuthenticationCredentials credentials_{kKey, kSecret};
  auth::Settings settings_{credentials_};
  std::shared_ptr<NetworkMock> network_;
  std::shared_ptr<olp::thread::TaskScheduler> task_scheduler_;
  std::unique_ptr<auth::TokenEndpoint> endpoint_;
};

TEST_F(TokenEndpointTest, RequestTokenUsingSystemTime) {
  // Default time source should be system time
  ASSERT_TRUE(settings_.use_system_time);

  ExpectNoTimestampRequest(*network_);

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kResponseValidJson));

  client::CancellationContext context;
  const auto token_response = endpoint_->RequestToken(context);

  ASSERT_TRUE(token_response);
  const auto& token_result = token_response.GetResult();

  const auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  EXPECT_LT(token_result.GetExpiryTime(), now + kMaxExpiryTime);
  EXPECT_GE(token_result.GetExpiryTime(), now + kMinExpiryTime);
  EXPECT_EQ(token_result.GetAccessToken(), kResponseToken);
}

TEST_F(TokenEndpointTest, RequestTokenUsingWrongSystemTime) {
  // Default time source should be system time
  ASSERT_TRUE(settings_.use_system_time);

  ExpectNoTimestampRequest(*network_);

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(MockWrongTimestamp())
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kResponseValidJson));

  client::CancellationContext context;
  const auto token_response = endpoint_->RequestToken(context);

  ASSERT_TRUE(token_response);
  const auto& token_result = token_response.GetResult();

  const auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  EXPECT_LT(token_result.GetExpiryTime(), now + kMaxExpiryTime);
  EXPECT_GE(token_result.GetExpiryTime(), now + kMinExpiryTime);
  EXPECT_EQ(token_result.GetAccessToken(), kResponseToken);
}

TEST_F(TokenEndpointTest, RequestTokenUsingServerTime) {
  PrepareEndpoint(false);

  ExpectTimestampRequest(*network_);

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kResponseValidJson));

  client::CancellationContext context;
  const auto token_response = endpoint_->RequestToken(context);

  ASSERT_TRUE(token_response);
  const auto& token_result = token_response.GetResult();

  const auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  EXPECT_LT(token_result.GetExpiryTime(), now + kMaxExpiryTime);
  EXPECT_GE(token_result.GetExpiryTime(), now + kMinExpiryTime);
  EXPECT_EQ(token_result.GetAccessToken(), kResponseToken);
}

TEST_F(TokenEndpointTest, TestInvalidResponses) {
  {
    SCOPED_TRACE("Invalid JSON");

    EXPECT_NO_FATAL_FAILURE(ExecuteRequestTokenWithError(
        http::HttpStatusCode::SERVICE_UNAVAILABLE, kErrorServiceUnavailable,
        kResponseInvalidJson));
  }

  {
    SCOPED_TRACE("No token");

    EXPECT_NO_FATAL_FAILURE(ExecuteRequestTokenWithError(
        http::HttpStatusCode::SERVICE_UNAVAILABLE, kErrorServiceUnavailable,
        kResponseNoToken));
  }

  {
    SCOPED_TRACE("Token type missing");

    EXPECT_NO_FATAL_FAILURE(ExecuteRequestTokenWithError(
        http::HttpStatusCode::SERVICE_UNAVAILABLE, kErrorServiceUnavailable,
        kResponseNoTokenType));
  }

  {
    SCOPED_TRACE("Missing expiry");

    EXPECT_NO_FATAL_FAILURE(ExecuteRequestTokenWithError(
        http::HttpStatusCode::SERVICE_UNAVAILABLE, kErrorServiceUnavailable,
        kResponseNoExpiry));
  }
}

TEST_F(TokenEndpointTest, TestHttpRequestErrorCodes) {
  int status;

  {
    SCOPED_TRACE("ACCEPTED");

    status = http::HttpStatusCode::ACCEPTED;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("CREATED");

    status = http::HttpStatusCode::CREATED;
    EXPECT_NO_FATAL_FAILURE(ExecuteRequestTokenWithError(
        status, http::HttpErrorToString(status), kResponseCreated));
  }

  {
    SCOPED_TRACE("NON_AUTHORITATIVE_INFORMATION");

    status = http::HttpStatusCode::NON_AUTHORITATIVE_INFORMATION;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("NO_CONTENT");

    status = http::HttpStatusCode::NO_CONTENT;
    EXPECT_NO_FATAL_FAILURE(ExecuteRequestTokenWithError(
        status, http::HttpErrorToString(status), kResponseNoContent));
  }

  {
    SCOPED_TRACE("RESET_CONTENT");

    status = http::HttpStatusCode::RESET_CONTENT;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("MULTIPLE_CHOICES");

    status = http::HttpStatusCode::PARTIAL_CONTENT;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("MULTIPLE_CHOICES");

    status = http::HttpStatusCode::MULTIPLE_CHOICES;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("MOVED_PERMANENTLY");

    status = http::HttpStatusCode::MOVED_PERMANENTLY;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("FOUND");

    status = http::HttpStatusCode::FOUND;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("SEE_OTHER");

    status = http::HttpStatusCode::SEE_OTHER;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("NOT_MODIFIED");

    status = http::HttpStatusCode::NOT_MODIFIED;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("USE_PROXY");

    status = http::HttpStatusCode::USE_PROXY;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("BAD_REQUEST");

    status = http::HttpStatusCode::BAD_REQUEST;
    EXPECT_NO_FATAL_FAILURE(ExecuteRequestTokenWithError(
        status, kErrorBadRequestFullMessage, kResponseBadRequest));
  }

  {
    SCOPED_TRACE("UNAUTHORIZED");

    status = http::HttpStatusCode::UNAUTHORIZED;
    EXPECT_NO_FATAL_FAILURE(ExecuteRequestTokenWithError(
        status, kErrorUnauthorizedFullMessage, kResponseUnauthorized));
  }

  {
    SCOPED_TRACE("PAYMENT_REQUIRED");

    status = http::HttpStatusCode::PAYMENT_REQUIRED;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("NOT_FOUND");

    status = http::HttpStatusCode::NOT_FOUND;
    EXPECT_NO_FATAL_FAILURE(ExecuteRequestTokenWithError(
        status, kErrorUserNotFoundFullMessage, kResponseNotFound));
  }

  {
    SCOPED_TRACE("METHOD_NOT_ALLOWED");

    status = http::HttpStatusCode::METHOD_NOT_ALLOWED;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("FORBIDDEN");

    status = http::HttpStatusCode::FORBIDDEN;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("NOT_ACCEPTABLE");

    status = http::HttpStatusCode::NOT_ACCEPTABLE;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("PROXY_AUTHENTICATION_REQUIRED");

    status = http::HttpStatusCode::PROXY_AUTHENTICATION_REQUIRED;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("REQUEST_TIMEOUT");

    status = http::HttpStatusCode::REQUEST_TIMEOUT;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("CONFLICT");

    status = http::HttpStatusCode::CONFLICT;
    EXPECT_NO_FATAL_FAILURE(ExecuteRequestTokenWithError(
        status, kErrorConflictFullMessage, kResponseConflict));
  }

  {
    SCOPED_TRACE("GONE");

    status = http::HttpStatusCode::GONE;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("LENGTH_REQUIRED");

    status = http::HttpStatusCode::LENGTH_REQUIRED;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("PRECONDITION_FAILED");

    status = http::HttpStatusCode::PRECONDITION_FAILED;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("REQUEST_ENTITY_TOO_LARGE");

    status = http::HttpStatusCode::REQUEST_ENTITY_TOO_LARGE;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("REQUEST_URI_TOO_LONG");

    status = http::HttpStatusCode::REQUEST_URI_TOO_LONG;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("UNSUPPORTED_MEDIA_TYPE");

    status = http::HttpStatusCode::UNSUPPORTED_MEDIA_TYPE;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("TOO_MANY_REQUESTS");

    status = http::HttpStatusCode::TOO_MANY_REQUESTS;
    EXPECT_NO_FATAL_FAILURE(ExecuteRequestTokenWithError(
        status, kErrorTooManyRequestsFullMessage, kResponseTooManyRequests));
  }

  {
    SCOPED_TRACE("INTERNAL_SERVER_ERROR");

    status = http::HttpStatusCode::INTERNAL_SERVER_ERROR;
    EXPECT_NO_FATAL_FAILURE(ExecuteRequestTokenWithError(
        status, kErrorInternalServerFullMessage, kResponseInternalServerError));
  }

  {
    SCOPED_TRACE("NOT_IMPLEMENTED");

    status = http::HttpStatusCode::NOT_IMPLEMENTED;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("BAD_GATEWAY");

    status = http::HttpStatusCode::BAD_GATEWAY;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("SERVICE_UNAVAILABLE");

    status = http::HttpStatusCode::SERVICE_UNAVAILABLE;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("GATEWAY_TIMEOUT");

    status = http::HttpStatusCode::GATEWAY_TIMEOUT;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("VERSION_NOT_SUPPORTED");

    status = http::HttpStatusCode::VERSION_NOT_SUPPORTED;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("Custom positive status");

    status = 100000;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }

  {
    SCOPED_TRACE("Custom negative status");

    status = -100000;
    EXPECT_NO_FATAL_FAILURE(
        ExecuteRequestTokenWithError(status, http::HttpErrorToString(status)));
  }
}

TEST_F(TokenEndpointTest, UniqueNonce) {
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

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .Times(3)
      .WillRepeatedly(testing::WithArgs<0, 2>(
          [&](http::NetworkRequest request, http::Network::Callback callback) {
            extract_nonce(request.GetHeaders());
            callback(http::NetworkResponse()
                         .WithRequestId(kRequestId)
                         .WithStatus(http::HttpStatusCode::TOO_MANY_REQUESTS));
            return http::SendOutcome(kRequestId);
          }));

  client::CancellationContext context;
  endpoint_->RequestToken(context);
  EXPECT_EQ(nonces.size(), kExpectedRetryCount);
}

TEST_F(TokenEndpointTest, RetrySettings) {
  settings_.retry_settings.max_attempts = kMaxRetryAttempts;
  settings_.retry_settings.timeout = kRetryTimeout;
  PrepareEndpoint();

  const auto retry_predicate = testing::Property(
      &http::NetworkRequest::GetSettings,
      testing::AllOf(
          testing::Property(&http::NetworkSettings::GetConnectionTimeout,
                            kRetryTimeout),
          testing::Property(&http::NetworkSettings::GetTransferTimeout,
                            kRetryTimeout)));

  EXPECT_CALL(*network_, Send(retry_predicate, _, _, _, _))
      .Times(kMaxRetryAttempts)
      .WillRepeatedly(ReturnHttpResponse(
          GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS)
              .WithError(kErrorTooManyRequestsFullMessage),
          kResponseTooManyRequests));

  client::CancellationContext context;
  const auto token_response = endpoint_->RequestToken(context);

  ASSERT_FALSE(token_response);
  EXPECT_EQ(token_response.GetError().GetHttpStatusCode(),
            http::HttpStatusCode::TOO_MANY_REQUESTS);
}

TEST_F(TokenEndpointTest, ResponseFitsRetryCondition) {
  settings_.retry_settings.retry_condition =
      [](const client::HttpResponse& http_response) {
        return http_response.GetStatus() ==
               http::HttpStatusCode::TOO_MANY_REQUESTS;
      };

  PrepareEndpoint();

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .Times(kExpectedRetryCount)
      .WillRepeatedly(ReturnHttpResponse(
          GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS)
              .WithError(kErrorTooManyRequestsFullMessage),
          kResponseTooManyRequests));

  client::CancellationContext context;
  const auto token_response = endpoint_->RequestToken(context);

  ASSERT_FALSE(token_response);
  EXPECT_EQ(token_response.GetError().GetHttpStatusCode(),
            http::HttpStatusCode::TOO_MANY_REQUESTS);
}

TEST_F(TokenEndpointTest, ResponseDoesNotFitRetryCondition) {
  settings_.retry_settings.retry_condition =
      [](const client::HttpResponse& http_response) {
        return http_response.GetStatus() !=
               http::HttpStatusCode::TOO_MANY_REQUESTS;
      };

  PrepareEndpoint();

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS)
              .WithError(kErrorTooManyRequestsFullMessage),
          kResponseTooManyRequests));

  client::CancellationContext context;
  const auto token_response = endpoint_->RequestToken(context);

  ASSERT_FALSE(token_response);
  EXPECT_EQ(token_response.GetError().GetHttpStatusCode(),
            http::HttpStatusCode::TOO_MANY_REQUESTS);
}

TEST_F(TokenEndpointTest, OkRetryCondition) {
  // The request ended up with `OK` status should not be retriggered even if
  // `retry_condition` is `true` for this `HttpResponse`.

  settings_.retry_settings.retry_condition =
      [](const client::HttpResponse& http_response) {
        return http_response.GetStatus() == http::HttpStatusCode::OK;
      };

  PrepareEndpoint();

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kResponseValidJson));

  client::CancellationContext context;
  const auto token_response = endpoint_->RequestToken(context);

  ASSERT_TRUE(token_response);
  EXPECT_EQ(token_response.GetResult().GetAccessToken(), kResponseToken);
}

TEST_F(TokenEndpointTest, Timeout) {
  settings_.retry_settings.timeout = kMinTimeout;
  PrepareEndpoint();

  std::future<void> async_finish_future;

  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(testing::WithArg<2>([&](http::Network::Callback callback) {
        async_finish_future = std::async(std::launch::async, [=]() {
          // Oversleep the timeout period.
          std::this_thread::sleep_for(
              std::chrono::seconds(settings_.retry_settings.timeout * 2));
          callback(http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(kRequestId));
        });

        return http::SendOutcome(kRequestId);
      }));

  EXPECT_CALL(*network_, Cancel(_)).Times(1);

  client::CancellationContext context;
  const auto token_response = endpoint_->RequestToken(context);

  ASSERT_EQ(async_finish_future.wait_for(kWaitTimeout),
            std::future_status::ready);
  ASSERT_FALSE(token_response);
  EXPECT_EQ(token_response.GetError().GetHttpStatusCode(),
            static_cast<int>(http::ErrorCode::TIMEOUT_ERROR));
  EXPECT_EQ(token_response.GetError().GetMessage(), kErrorTimeOut);
}

TEST_F(TokenEndpointTest, Cancel) {
  std::future<void> async_finish_future;
  std::promise<void> network_wait_promise;

  client::CancellationContext context;
  EXPECT_CALL(*network_, Send(IsPostRequest(kRequestAuth), _, _, _, _))
      .WillOnce(testing::WithArg<2>([&](http::Network::Callback callback) {
        async_finish_future = std::async(std::launch::async, [&, callback]() {
          std::this_thread::sleep_for(std::chrono::seconds(kMinTimeout));
          context.CancelOperation();

          EXPECT_EQ(network_wait_promise.get_future().wait_for(kWaitTimeout),
                    std::future_status::ready);

          callback(http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(kRequestId));
        });

        return http::SendOutcome(kRequestId);
      }));

  EXPECT_CALL(*network_, Cancel(kRequestId)).Times(1);

  const auto token_response = endpoint_->RequestToken(context);
  network_wait_promise.set_value();

  ASSERT_EQ(async_finish_future.wait_for(kWaitTimeout),
            std::future_status::ready);
  ASSERT_FALSE(token_response);
  EXPECT_EQ(token_response.GetError().GetHttpStatusCode(),
            static_cast<int>(http::ErrorCode::CANCELLED_ERROR));
  EXPECT_EQ(token_response.GetError().GetMessage(), kErrorCancelled);
}

PORTING_POP_WARNINGS()
