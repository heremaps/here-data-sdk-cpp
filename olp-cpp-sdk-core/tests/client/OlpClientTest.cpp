/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <chrono>
#include <future>
#include <queue>
#include <string>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/Network.h>
#include <olp/core/http/NetworkConstants.h>
#include <olp/core/logging/Log.h>

namespace {
using olp::client::HttpResponse;
using olp::http::NetworkRequest;
using ::testing::_;
namespace http = olp::http;

enum class CallApiType { ASYNC, SYNC };

static constexpr auto kCallbackSleepTime = std::chrono::milliseconds(50);
static constexpr auto kCallbackWaitTime = std::chrono::seconds(10);
static const auto kToManyRequestResponse =
    http::NetworkResponse()
        .WithStatus(http::HttpStatusCode::TOO_MANY_REQUESTS)
        .WithError("Too many request, slow down!");

class CallApiWrapper {
 public:
  virtual ~CallApiWrapper() = default;

  virtual HttpResponse CallApi(
      std::string path, std::string method,
      std::multimap<std::string, std::string> query_params,
      std::multimap<std::string, std::string> header_params,
      std::multimap<std::string, std::string> form_params,
      std::shared_ptr<std::vector<unsigned char>> post_body,
      std::string content_type,
      olp::client::CancellationContext context = {}) const = 0;
};

template <CallApiType ApiType>
class CallApiGeneric : public CallApiWrapper {
 public:
  CallApiGeneric(const olp::client::OlpClient& client) : client_{client} {}

  HttpResponse CallApi(
      std::string path, std::string method,
      std::multimap<std::string, std::string> query_params,
      std::multimap<std::string, std::string> header_params,
      std::multimap<std::string, std::string> form_params,
      std::shared_ptr<std::vector<unsigned char>> post_body,
      std::string content_type,
      olp::client::CancellationContext context) const override {
    if (ApiType == CallApiType::ASYNC) {
      // Async callback
      std::promise<HttpResponse> promise;
      olp::client::NetworkAsyncCallback callback =
          [&](HttpResponse http_response) {
            promise.set_value(std::move(http_response));
          };

      context.ExecuteOrCancelled(
          [&]() {
            return client_.CallApi(
                std::move(path), std::move(method), std::move(query_params),
                std::move(header_params), std::move(form_params),
                std::move(post_body), std::move(content_type), callback);
          },
          [&]() {
            callback({static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
                      "Operation is cancelled."});
          });

      return promise.get_future().get();
    } else {
      // Sync callback
      return client_.CallApi(std::move(path), std::move(method),
                             std::move(query_params), std::move(header_params),
                             std::move(form_params), std::move(post_body),
                             std::move(content_type), context);
    }
  }

  const olp::client::OlpClient& client_;
};

class NetworkMock : public olp::http::Network {
 public:
  MOCK_METHOD(olp::http::SendOutcome, Send,
              (olp::http::NetworkRequest request,
               olp::http::Network::Payload payload,
               olp::http::Network::Callback callback,
               olp::http::Network::HeaderCallback header_callback,
               olp::http::Network::DataCallback data_callback),
              (override));

  MOCK_METHOD(void, Cancel, (olp::http::RequestId id), (override));
};

std::ostream& operator<<(std::ostream& os, const CallApiType call_type) {
  switch (call_type) {
    case CallApiType::ASYNC:
      os << "ASYNC";
      break;
    case CallApiType::SYNC:
      os << "SYNC";
      break;
    default:
      os << "UNKNOWN";
      break;
  }
  return os;
}

class OlpClientTest : public ::testing::TestWithParam<CallApiType> {
 protected:
  void SetUp() override {
    network_ = std::make_shared<NetworkMock>();
    client_settings_.network_request_handler = network_;
    switch (GetParam()) {
      case CallApiType::ASYNC:
        call_wrapper_ =
            std::make_shared<CallApiGeneric<CallApiType::ASYNC>>(client_);
        break;
      case CallApiType::SYNC:
        call_wrapper_ =
            std::make_shared<CallApiGeneric<CallApiType::SYNC>>(client_);
        break;
      default:
        ADD_FAILURE() << "Invalid type of CallApi wrapper";
        break;
    }
  }

  olp::client::OlpClientSettings client_settings_;
  olp::client::OlpClient client_;
  std::shared_ptr<CallApiWrapper> call_wrapper_;
  std::shared_ptr<NetworkMock> network_;
};

TEST_P(OlpClientTest, NumberOfAttempts) {
  auto network = network_;
  client_settings_.retry_settings.max_attempts = 5;
  client_settings_.retry_settings.retry_condition =
      [](const olp::client::HttpResponse&) { return true; };
  client_.SetSettings(client_settings_);

  std::vector<std::future<void>> futures;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(6)
      .WillRepeatedly(
          [&](olp::http::NetworkRequest /*request*/,
              olp::http::Network::Payload payload,
              olp::http::Network::Callback callback,
              olp::http::Network::HeaderCallback /*header_callback*/,
              olp::http::Network::DataCallback /*data_callback*/) {
            auto current_request_id = request_id++;
            futures.emplace_back(std::async(std::launch::async, [=]() {
              std::this_thread::sleep_for(kCallbackSleepTime);
              auto response = kToManyRequestResponse;
              response.WithRequestId(current_request_id);
              const auto& error = response.GetError();

              payload->seekp(0, std::ios_base::end);
              payload->write(error.c_str(), error.size());
              payload->seekp(0);
              callback(response);
            }));
            return olp::http::SendOutcome(current_request_id);
          });

  auto response = call_wrapper_->CallApi({}, "GET", {}, {}, {}, nullptr, {});

  for (auto& future : futures) {
    future.wait();
  }

  ASSERT_EQ(kToManyRequestResponse.GetStatus(), response.GetStatus());
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, ZeroAttempts) {
  auto network = network_;
  client_settings_.retry_settings.max_attempts = 0;
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse&) { return true; });
  client_.SetSettings(client_settings_);

  std::vector<std::future<void>> futures;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest /*request*/,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        auto current_request_id = request_id++;
        futures.emplace_back(std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          auto response = kToManyRequestResponse;
          response.WithRequestId(current_request_id);
          const auto& error = response.GetError();

          payload->seekp(0, std::ios_base::end);
          payload->write(error.c_str(), error.size());
          payload->seekp(0);
          callback(response);
        }));
        return olp::http::SendOutcome(current_request_id);
      });

  auto response = call_wrapper_->CallApi({}, "GET", {}, {}, {}, nullptr, {});

  for (auto& future : futures) {
    future.wait();
  }

  ASSERT_EQ(kToManyRequestResponse.GetStatus(), response.GetStatus());
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, DefaultRetryCondition) {
  auto network = network_;
  olp::http::RequestId request_id = 5;
  std::vector<std::future<void>> futures;

  // retry for IO, offline, timeout, network overload error codes and 429, all
  // 5xx http status codes.
  auto attempt_statuses =
      std::queue<int>{{-1,  -4,  -7,  -8,  429, 500, 501, 502, 503, 504,
                       505, 506, 507, 508, 509, 510, 511, 598, 599, 200}};
  client_settings_.retry_settings.max_attempts = attempt_statuses.size();
  client_settings_.retry_settings.backdown_strategy =
      [](std::chrono::milliseconds, size_t) {
        return std::chrono::milliseconds(0);
      };
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(static_cast<int>(attempt_statuses.size()))
      .WillRepeatedly(
          [&](olp::http::NetworkRequest /*request*/,
              olp::http::Network::Payload payload,
              olp::http::Network::Callback callback,
              olp::http::Network::HeaderCallback /*header_callback*/,
              olp::http::Network::DataCallback /*data_callback*/) {
            if (attempt_statuses.empty()) {
              ADD_FAILURE_AT(__FILE__, __LINE__) << "Unexpected retry attempt";
              return olp::http::SendOutcome(
                  olp::http::ErrorCode::UNKNOWN_ERROR);
            }

            auto status = attempt_statuses.front();
            attempt_statuses.pop();

            auto current_request_id = request_id++;
            futures.emplace_back(std::async(std::launch::async, [=]() {
              std::this_thread::sleep_for(kCallbackSleepTime);
              auto response =
                  olp::http::NetworkResponse().WithStatus(status).WithRequestId(
                      current_request_id);

              std::string error = "Error, please check HTTP status code";
              payload->seekp(0, std::ios_base::end);
              payload->write(error.c_str(), error.size());
              payload->seekp(0);
              callback(response);
            }));

            return olp::http::SendOutcome(current_request_id);
          });

  auto response = call_wrapper_->CallApi({}, "GET", {}, {}, {}, nullptr, {});

  for (auto& future : futures) {
    future.wait();
  }

  ASSERT_EQ(http::HttpStatusCode::OK, response.GetStatus());
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, RetryCondition) {
  int current_attempt = 0;
  const int good_attempt = 4;
  auto network = network_;
  client_settings_.retry_settings.max_attempts = 6;
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse& response) {
        return response.status == http::HttpStatusCode::TOO_MANY_REQUESTS;
      });
  client_.SetSettings(client_settings_);

  std::vector<std::future<void>> futures;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(good_attempt)
      .WillRepeatedly(
          [&](olp::http::NetworkRequest /*request*/,
              olp::http::Network::Payload payload,
              olp::http::Network::Callback callback,
              olp::http::Network::HeaderCallback /*header_callback*/,
              olp::http::Network::DataCallback /*data_callback*/) {
            auto current_request_id = request_id++;
            current_attempt++;

            futures.emplace_back(std::async(std::launch::async, [=]() {
              std::this_thread::sleep_for(kCallbackSleepTime);
              auto response = olp::http::NetworkResponse().WithRequestId(
                  current_request_id);

              if (current_attempt == good_attempt) {
                response.WithStatus(http::HttpStatusCode::OK);
              } else {
                response.WithError(kToManyRequestResponse.GetError())
                    .WithStatus(kToManyRequestResponse.GetStatus());

                const auto& error = response.GetError();
                payload->seekp(0, std::ios_base::end);
                payload->write(error.c_str(), error.size());
                payload->seekp(0);
              }

              callback(std::move(response));
            }));

            return olp::http::SendOutcome(current_request_id);
          });

  auto response = call_wrapper_->CallApi({}, "GET", {}, {}, {}, nullptr, {});

  for (auto& future : futures) {
    future.wait();
  }

  ASSERT_EQ(http::HttpStatusCode::OK, response.GetStatus());
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, RetryWithExponentialBackdownStrategy) {
  auto network = network_;
  const std::chrono::milliseconds::rep kInitialBackdownPeriod = 100;
  size_t expected_retry_count = 0;
  std::vector<std::chrono::milliseconds::rep> wait_times = {
      kInitialBackdownPeriod};

  auto& retry_settings = client_settings_.retry_settings;

  // Setup retry settings:
  retry_settings.initial_backdown_period = kInitialBackdownPeriod;
  retry_settings.retry_condition = [](const olp::client::HttpResponse&) {
    return true;
  };
  retry_settings.backdown_strategy =
      [&](std::chrono::milliseconds period,
          size_t retry_count) -> std::chrono::milliseconds {
    EXPECT_EQ(kInitialBackdownPeriod, period.count());
    EXPECT_EQ(++expected_retry_count, retry_count);

    const auto wait_time = olp::client::ExponentialBackdownStrategy()(
        std::chrono::milliseconds(kInitialBackdownPeriod), retry_count);
    wait_times.push_back(wait_time.count());
    return wait_time;
  };
  client_.SetSettings(client_settings_);

  const auto requests_count = retry_settings.max_attempts + 1;
  std::vector<std::chrono::system_clock::time_point> timestamps;
  std::vector<std::future<void>> futures;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(requests_count)
      .WillRepeatedly(
          [&](olp::http::NetworkRequest /*request*/,
              olp::http::Network::Payload payload,
              olp::http::Network::Callback callback,
              olp::http::Network::HeaderCallback /*header_callback*/,
              olp::http::Network::DataCallback /*data_callback*/) {
            timestamps.push_back(std::chrono::system_clock::now());
            auto current_request_id = request_id++;

            futures.emplace_back(std::async(std::launch::async, [=]() {
              std::this_thread::sleep_for(kCallbackSleepTime);
              auto response = kToManyRequestResponse;
              response.WithRequestId(current_request_id);
              const auto& error = response.GetError();

              payload->seekp(0, std::ios_base::end);
              payload->write(error.c_str(), error.size());
              payload->seekp(0);
              callback(response);
            }));
            return olp::http::SendOutcome(current_request_id);
          });

  auto response = call_wrapper_->CallApi({}, "GET", {}, {}, {}, nullptr, {});

  ASSERT_EQ(kToManyRequestResponse.GetStatus(), response.GetStatus());
  ASSERT_EQ(client_settings_.retry_settings.max_attempts, expected_retry_count);

  for (auto& future : futures) {
    future.wait();
  }

  // The duration between retries should match actual wait time from backdown
  // policy
  for (size_t i = 1; i < timestamps.size(); ++i) {
    ASSERT_GE((timestamps[i] - timestamps[i - 1]).count(), wait_times[i - 1]);
  }

  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, RetryTimeout) {
  const size_t kMaxRetries = 3;
  const size_t kSuccessfulAttempt = kMaxRetries + 1;

  // Setup retry settings:
  client_settings_.retry_settings.initial_backdown_period = 400;  // msec
  client_settings_.retry_settings.max_attempts = kMaxRetries;
  client_settings_.retry_settings.timeout = 1;  // seconds
  client_settings_.retry_settings.retry_condition = [](const HttpResponse&) {
    return true;
  };
  client_settings_.retry_settings.backdown_strategy =
      [](std::chrono::milliseconds initial_backdown_period,
         size_t retry_count) -> std::chrono::milliseconds {
    return initial_backdown_period *
           static_cast<size_t>(std::pow(2, retry_count));
  };
  client_.SetSettings(client_settings_);

  auto network = network_;
  size_t current_attempt = 0;
  std::vector<std::future<void>> futures;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillRepeatedly(
          [&](olp::http::NetworkRequest /*request*/,
              olp::http::Network::Payload payload,
              olp::http::Network::Callback callback,
              olp::http::Network::HeaderCallback /*header_callback*/,
              olp::http::Network::DataCallback /*data_callback*/) {
            auto current_request_id = request_id++;
            current_attempt++;

            futures.emplace_back(std::async(std::launch::async, [=]() {
              std::this_thread::sleep_for(kCallbackSleepTime);
              auto response = olp::http::NetworkResponse().WithRequestId(
                  current_request_id);

              // the test shouldn't reach the last retry due to timeout
              // restrictions in retry settings
              if (current_attempt == kSuccessfulAttempt) {
                ADD_FAILURE_AT(__FILE__, __LINE__)
                    << "Unexpected retry attempt";
                response.WithStatus(http::HttpStatusCode::OK);
              } else {
                response.WithError(kToManyRequestResponse.GetError())
                    .WithStatus(kToManyRequestResponse.GetStatus());

                const auto& error = response.GetError();
                payload->seekp(0, std::ios_base::end);
                payload->write(error.c_str(), error.size());
                payload->seekp(0);
              }

              callback(std::move(response));
            }));

            return olp::http::SendOutcome(current_request_id);
          });

  auto response = call_wrapper_->CallApi({}, "GET", {}, {}, {}, nullptr, {});

  for (auto& future : futures) {
    future.wait();
  }

  ASSERT_EQ(kToManyRequestResponse.GetStatus(), response.GetStatus());
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, Timeout) {
  client_settings_.retry_settings.timeout = 100;
  client_settings_.retry_settings.max_attempts = 0;
  int timeout = 0;
  auto network = network_;
  client_.SetSettings(client_settings_);

  std::vector<std::future<void>> futures;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillRepeatedly(
          [&](olp::http::NetworkRequest request,
              olp::http::Network::Payload payload,
              olp::http::Network::Callback callback,
              olp::http::Network::HeaderCallback /*header_callback*/,
              olp::http::Network::DataCallback /*data_callback*/) {
            timeout = request.GetSettings().GetConnectionTimeout();
            auto current_request_id = request_id++;

            futures.emplace_back(std::async(std::launch::async, [=]() {
              std::this_thread::sleep_for(kCallbackSleepTime);
              auto response = kToManyRequestResponse;
              response.WithRequestId(current_request_id);
              const auto& error = response.GetError();

              payload->seekp(0, std::ios_base::end);
              payload->write(error.c_str(), error.size());
              payload->seekp(0);
              callback(response);
            }));

            return olp::http::SendOutcome(current_request_id);
          });

  auto response = call_wrapper_->CallApi({}, "GET", {}, {}, {}, nullptr, {});

  for (auto& future : futures) {
    future.wait();
  }

  ASSERT_EQ(client_settings_.retry_settings.timeout, timeout);
  ASSERT_EQ(kToManyRequestResponse.GetStatus(), response.GetStatus());
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, Proxy) {
  auto network = network_;
  client_settings_.retry_settings.timeout = 100;
  auto expected_settings =
      olp::http::NetworkProxySettings()
          .WithHostname("somewhere")
          .WithPort(1080)
          .WithType(olp::http::NetworkProxySettings::Type::HTTP)
          .WithUsername("username1")
          .WithPassword("1");

  client_settings_.proxy_settings = expected_settings;
  olp::http::NetworkProxySettings result_settings;
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload /*payload*/,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        result_settings = request.GetSettings().GetProxySettings();
        auto current_request_id = request_id++;

        future = std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          callback(olp::http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(current_request_id));
        });

        return olp::http::SendOutcome(current_request_id);
      });

  auto response = call_wrapper_->CallApi({}, "GET", {}, {}, {}, nullptr, {});

  future.wait();

  ASSERT_EQ(expected_settings.GetHostname(), result_settings.GetHostname());
  ASSERT_EQ(expected_settings.GetPassword(), result_settings.GetPassword());
  ASSERT_EQ(expected_settings.GetPort(), result_settings.GetPort());
  ASSERT_EQ(expected_settings.GetUsername(), result_settings.GetUsername());
  ASSERT_EQ(expected_settings.GetPassword(), result_settings.GetPassword());

  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, EmptyProxy) {
  auto network = network_;
  client_settings_.retry_settings.timeout = 100;
  client_settings_.proxy_settings = boost::none;
  ASSERT_FALSE(client_settings_.proxy_settings);

  olp::http::NetworkProxySettings result_settings;
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload /*payload*/,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        result_settings = request.GetSettings().GetProxySettings();
        auto current_request_id = request_id++;

        future = std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          callback(olp::http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(current_request_id));
        });

        return olp::http::SendOutcome(current_request_id);
      });

  auto response = call_wrapper_->CallApi({}, "GET", {}, {}, {}, nullptr, {});

  future.wait();

  ASSERT_FALSE(result_settings.GetType() !=
               olp::http::NetworkProxySettings::Type::NONE);
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, HttpResponse) {
  auto network = network_;
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest /*request*/,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        auto current_request_id = request_id++;
        future = std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          *payload << "content";
          callback(olp::http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(current_request_id));
        });

        return olp::http::SendOutcome(current_request_id);
      });

  auto response = call_wrapper_->CallApi({}, "GET", {}, {}, {}, nullptr, {});

  future.wait();

  std::string response_payload;
  response.GetResponse(response_payload);
  ASSERT_EQ("content", response_payload);
  ASSERT_EQ(http::HttpStatusCode::OK, response.GetStatus());

  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, Paths) {
  auto network = network_;
  client_.SetBaseUrl("here.com");
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload /*payload*/,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        EXPECT_EQ("here.com/index", request.GetUrl());
        auto current_request_id = request_id++;

        future = std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          callback(olp::http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(current_request_id));
        });

        return olp::http::SendOutcome(current_request_id);
      });

  auto response =
      call_wrapper_->CallApi("/index", "GET", {}, {}, {}, nullptr, {});

  future.wait();
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, Method) {
  auto network = network_;
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  const auto methods = std::vector<std::string>(
      {"GET", "POST", "PUT", "DELETE", "OPTIONS", "PATCH", "HEAD"});
  const auto expected = std::vector<NetworkRequest::HttpVerb>(
      {NetworkRequest::HttpVerb::GET, NetworkRequest::HttpVerb::POST,
       NetworkRequest::HttpVerb::PUT, NetworkRequest::HttpVerb::DEL,
       NetworkRequest::HttpVerb::OPTIONS, NetworkRequest::HttpVerb::PATCH,
       NetworkRequest::HttpVerb::HEAD});
  NetworkRequest::HttpVerb expected_verb = expected[0];

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(static_cast<int>(methods.size()))
      .WillRepeatedly(
          [&](olp::http::NetworkRequest request,
              olp::http::Network::Payload /*payload*/,
              olp::http::Network::Callback callback,
              olp::http::Network::HeaderCallback /*header_callback*/,
              olp::http::Network::DataCallback /*data_callback*/) {
            EXPECT_EQ(expected_verb, request.GetVerb());
            auto current_request_id = request_id++;

            future = std::async(std::launch::async, [=]() {
              std::this_thread::sleep_for(kCallbackSleepTime);
              callback(olp::http::NetworkResponse()
                           .WithStatus(http::HttpStatusCode::OK)
                           .WithRequestId(current_request_id));
            });

            return olp::http::SendOutcome(current_request_id);
          });

  for (size_t idx = 0; idx < methods.size(); ++idx) {
    const auto& method = methods[idx];
    expected_verb = expected[idx];

    SCOPED_TRACE(testing::Message() << "Method=" << method);

    auto response = call_wrapper_->CallApi({}, method, {}, {}, {}, nullptr, {});
    future.wait();
  }

  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, QueryParam) {
  auto network = network_;
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload /*payload*/,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        EXPECT_EQ("index?var1=&var2=2", request.GetUrl());
        auto current_request_id = request_id++;

        future = std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          callback(olp::http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(current_request_id));
        });

        return olp::http::SendOutcome(current_request_id);
      });

  std::multimap<std::string, std::string> query_params = {{"var1", ""},
                                                          {"var2", "2"}};

  auto response =
      call_wrapper_->CallApi("index", "GET", query_params, {}, {}, nullptr, {});

  future.wait();
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, HeaderParams) {
  auto network = network_;
  std::multimap<std::string, std::string> header_params = {{"head1", "value1"},
                                                           {"head2", "value2"}};
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload /*payload*/,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        const auto& result_headers = request.GetHeaders();
        auto current_request_id = request_id++;

        EXPECT_LE(2u, result_headers.size());
        if (result_headers.size() >= 2u) {
          for (auto& entry : result_headers) {
            if (entry.first == "head1") {
              EXPECT_EQ("value1", entry.second);
            } else if (entry.first == "head2") {
              EXPECT_EQ("value2", entry.second);
            }
          }
        }

        future = std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          callback(olp::http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(current_request_id));
        });

        return olp::http::SendOutcome(current_request_id);
      });

  auto response =
      call_wrapper_->CallApi({}, "GET", {}, header_params, {}, nullptr, {});

  future.wait();
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, DefaultHeaderParams) {
  auto network = network_;
  client_.GetMutableDefaultHeaders().insert(std::make_pair("head1", "value1"));
  client_.GetMutableDefaultHeaders().insert(std::make_pair("head2", "value2"));
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload /*payload*/,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        const auto& result_headers = request.GetHeaders();
        auto current_request_id = request_id++;

        EXPECT_LE(2u, result_headers.size());
        if (result_headers.size() >= 2u) {
          for (auto& entry : result_headers) {
            if (entry.first == "head1") {
              EXPECT_EQ("value1", entry.second);
            } else if (entry.first == "head2") {
              EXPECT_EQ("value2", entry.second);
            }
          }
        }

        future = std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          callback(olp::http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(current_request_id));
        });

        return olp::http::SendOutcome(current_request_id);
      });

  auto response = call_wrapper_->CallApi({}, "GET", {}, {}, {}, nullptr, {});

  future.wait();
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, CombineHeaderParams) {
  auto network = network_;
  client_.GetMutableDefaultHeaders().insert(std::make_pair("head1", "value1"));
  client_.GetMutableDefaultHeaders().insert(std::make_pair("head2", "value2"));
  std::multimap<std::string, std::string> header_params;
  header_params.insert(std::make_pair("head3", "value3"));
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload /*payload*/,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        const auto& result_headers = request.GetHeaders();
        auto current_request_id = request_id++;

        EXPECT_LE(3u, result_headers.size());
        if (result_headers.size() >= 3u) {
          for (auto& entry : result_headers) {
            if (entry.first == "head1") {
              EXPECT_EQ("value1", entry.second);
            } else if (entry.first == "head2") {
              EXPECT_EQ("value2", entry.second);
            } else if (entry.first == "head3") {
              EXPECT_EQ("value3", entry.second);
            }
          }
        }

        future = std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          callback(olp::http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(current_request_id));
        });

        return olp::http::SendOutcome(current_request_id);
      });

  auto response =
      call_wrapper_->CallApi({}, "GET", {}, header_params, {}, nullptr, {});

  future.wait();
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, QueryMultiParams) {
  std::string uri;
  std::vector<std::pair<std::string, std::string>> headers;
  auto network = network_;
  client_.SetSettings(client_settings_);

  std::multimap<std::string, std::string> query_params = {
      {"a", "a1"}, {"b", "b1"}, {"b", "b2"},
      {"c", "c1"}, {"c", "c2"}, {"c", "c3"}};

  std::multimap<std::string, std::string> header_params = {
      {"z", "z1"}, {"y", "y1"}, {"y", "y2"},
      {"x", "x1"}, {"x", "x2"}, {"x", "x3"}};

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload /*payload*/,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        uri = request.GetUrl();
        headers = request.GetHeaders();
        auto current_request_id = request_id++;

        future = std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          callback(olp::http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(current_request_id));
        });

        return olp::http::SendOutcome(current_request_id);
      });

  auto response = call_wrapper_->CallApi({}, {}, query_params, header_params,
                                         {}, nullptr, {});
  // query test
  for (auto q : query_params) {
    std::string param_equal_value = q.first + "=" + q.second;
    ASSERT_NE(uri.find(param_equal_value), std::string::npos);
  }
  ASSERT_EQ(uri.find("not=present"), std::string::npos);

  // headers test
  ASSERT_LE(6, headers.size());
  for (auto p : header_params) {
    ASSERT_TRUE(std::find_if(headers.begin(), headers.end(),
                             [p](decltype(p) el) { return el == p; }) !=
                headers.end());
  }

  decltype(header_params)::value_type new_value{"added", "new"};
  header_params.insert(new_value);
  ASSERT_FALSE(
      std::find_if(headers.begin(), headers.end(), [&](decltype(new_value) el) {
        return el == new_value;
      }) != headers.end());

  future.wait();
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, Content) {
  auto network = network_;
  client_.GetMutableDefaultHeaders().insert(std::make_pair("head1", "value1"));
  std::multimap<std::string, std::string> header_params = {{"head3", "value3"}};
  const std::string content_string = "something";
  const auto content = std::make_shared<std::vector<unsigned char>>(
      content_string.begin(), content_string.end());
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload /*payload*/,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        const auto& result_headers = request.GetHeaders();
        const auto& result_content = request.GetBody();
        auto current_request_id = request_id++;

        EXPECT_LE(3u, result_headers.size());
        for (auto& entry : result_headers) {
          if (entry.first == "head1") {
            EXPECT_EQ("value1", entry.second);
          } else if (entry.first == "head3") {
            EXPECT_EQ("value3", entry.second);
          } else if (entry.first == "Content-Type") {
            EXPECT_EQ("plain-text", entry.second);
          }
        }

        EXPECT_TRUE(result_content);
        EXPECT_EQ(*content, *result_content);

        future = std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          callback(olp::http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(current_request_id));
        });

        return olp::http::SendOutcome(current_request_id);
      });

  auto response = call_wrapper_->CallApi({}, "GET", {}, header_params, {},
                                         content, "plain-text");

  future.wait();
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, CancelBeforeResponse) {
  auto network = network_;
  auto cancel_wait = std::make_shared<std::promise<bool>>();
  auto network_wait = std::make_shared<std::promise<bool>>();
  auto cancelled = std::make_shared<std::atomic_bool>(false);
  constexpr int kExpectedError =
      static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR);
  client_.SetBaseUrl("https://www.google.com");
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest /*request*/,
                    olp::http::Network::Payload /*payload*/,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        auto current_request_id = request_id++;

        future = std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          network_wait->set_value(true);
          cancel_wait->get_future().get();

          // Although we send OK back, we should receive CANCELLED_ERROR from
          // PendingUrlRequests
          callback(olp::http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(current_request_id));
        });

        return olp::http::SendOutcome(current_request_id);
      });

  EXPECT_CALL(*network, Cancel(request_id)).WillOnce([=](olp::http::RequestId) {
    cancelled->store(true);
  });

  olp::client::CancellationContext context;

  auto response_future = std::async(std::launch::async, [&]() {
    return call_wrapper_->CallApi({}, "GET", {}, {}, {}, nullptr, {}, context);
  });

  // Wait for Network call and cancel it
  network_wait->get_future().wait();
  context.CancelOperation();

  // Release Network waiting and return result
  cancel_wait->set_value(true);
  ASSERT_EQ(std::future_status::ready,
            response_future.wait_for(kCallbackWaitTime));
  EXPECT_TRUE(cancelled->load());

  // Check response has cancelled error
  auto result = response_future.get();
  EXPECT_EQ(kExpectedError, result.GetStatus());

  future.wait();
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, HeadersCallbackAfterCancel) {
  auto network = network_;
  auto cancel_wait = std::make_shared<std::promise<bool>>();
  auto network_wait = std::make_shared<std::promise<bool>>();
  auto cancelled = std::make_shared<std::atomic_bool>(false);
  client_.SetBaseUrl("https://www.google.com");
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  olp::http::Network::HeaderCallback headers_cb;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest /*request*/,
                    olp::http::Network::Payload /*payload*/,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback /*data_callback*/) {
        headers_cb = std::move(header_callback);
        auto current_request_id = request_id++;

        future = std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          network_wait->set_value(true);
          cancel_wait->get_future().get();

          // Although we send OK back, we should receive CANCELLED_ERROR from
          // PendingUrlRequests
          callback(olp::http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(current_request_id));
        });

        return olp::http::SendOutcome(current_request_id);
      });

  EXPECT_CALL(*network, Cancel(request_id)).WillOnce([=](olp::http::RequestId) {
    cancelled->store(true);
  });

  olp::client::CancellationContext context;

  auto response_future = std::async(std::launch::async, [&]() {
    return call_wrapper_->CallApi({}, "GET", {}, {{"header", "header"}}, {},
                                  nullptr, {}, context);
  });

  network_wait->get_future().wait();
  context.CancelOperation();
  cancel_wait->set_value(true);

  ASSERT_EQ(std::future_status::ready,
            response_future.wait_for(kCallbackWaitTime));

  // call headers callback after call was canceled
  headers_cb("header", "header");

  EXPECT_TRUE(cancelled->load());

  future.wait();
  testing::Mock::VerifyAndClearExpectations(network.get());
}

// Test is valid only valid for sync api
TEST_P(OlpClientTest, CancelBeforeExecution) {
  client_.SetBaseUrl("https://www.google.com");
  auto network = network_;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _)).Times(0);
  olp::client::CancellationContext context;
  context.CancelOperation();
  auto response = client_.CallApi({}, "GET", {}, {}, {}, nullptr, {}, context);
  ASSERT_EQ(response.status,
            static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR));
}

TEST_P(OlpClientTest, CancelAfterCompletion) {
  client_.SetBaseUrl("https://www.google.com");
  auto network = network_;
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  {
    SCOPED_TRACE("Merged");

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .WillOnce([&](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/) {
          auto current_request_id = request_id++;

          future = std::async(std::launch::async, [=]() {
            std::this_thread::sleep_for(kCallbackSleepTime);
            callback(olp::http::NetworkResponse()
                         .WithStatus(http::HttpStatusCode::OK)
                         .WithRequestId(current_request_id));
          });

          return olp::http::SendOutcome(current_request_id);
        });

    EXPECT_CALL(*network, Cancel(request_id)).Times(0);

    std::promise<olp::client::HttpResponse> promise;
    auto cancel_token =
        client_.CallApi({}, "GET", {}, {}, {}, nullptr, {},
                        [&](olp::client::HttpResponse http_response) {
                          promise.set_value(std::move(http_response));
                        });

    auto response = promise.get_future().get();
    ASSERT_EQ(http::HttpStatusCode::OK, response.GetStatus());

    // When cancelled after it finished, nothing should happen
    cancel_token.Cancel();

    future.wait();
    testing::Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Not merged");

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .WillOnce([&](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/) {
          auto current_request_id = request_id++;

          future = std::async(std::launch::async, [=]() {
            std::this_thread::sleep_for(kCallbackSleepTime);
            callback(olp::http::NetworkResponse()
                         .WithStatus(http::HttpStatusCode::OK)
                         .WithRequestId(current_request_id));
          });

          return olp::http::SendOutcome(current_request_id);
        });

    const std::string content_string = "something";
    const auto content = std::make_shared<std::vector<unsigned char>>(
        content_string.begin(), content_string.end());

    EXPECT_CALL(*network, Cancel(request_id)).Times(0);

    std::promise<olp::client::HttpResponse> promise;
    auto cancel_token =
        client_.CallApi({}, "GET", {}, {}, {}, content, {},
                        [&](olp::client::HttpResponse http_response) {
                          promise.set_value(std::move(http_response));
                        });

    auto response = promise.get_future().get();
    ASSERT_EQ(http::HttpStatusCode::OK, response.GetStatus());

    // When cancelled after it finished, nothing should happen
    cancel_token.Cancel();

    future.wait();
    testing::Mock::VerifyAndClearExpectations(network.get());
  }
}

// Test only make sense for async api, as CancellationContext guards for
// double cancellation.
TEST_P(OlpClientTest, CancelDuplicate) {
  client_.SetBaseUrl("https://www.google.com");
  auto network = network_;
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  {
    SCOPED_TRACE("Merged");

    auto wait_for_cancel = std::make_shared<std::promise<bool>>();
    auto cancelled = std::make_shared<std::atomic_bool>(false);

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .WillOnce([&](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/) {
          auto current_request_id = request_id++;

          future = std::async(std::launch::async, [=]() {
            std::this_thread::sleep_for(kCallbackSleepTime);
            wait_for_cancel->get_future().get();
            callback(olp::http::NetworkResponse()
                         .WithStatus(http::HttpStatusCode::OK)
                         .WithRequestId(current_request_id));
          });

          return olp::http::SendOutcome(current_request_id);
        });

    EXPECT_CALL(*network, Cancel(request_id))
        .WillOnce([&](olp::http::RequestId) { cancelled->store(true); });

    std::promise<olp::client::HttpResponse> promise;
    auto callback = [&](olp::client::HttpResponse response) {
      promise.set_value(std::move(response));
    };

    auto cancel_token =
        client_.CallApi({}, "GET", {}, {}, {}, nullptr, {}, callback);

    // Cancel multiple times
    cancel_token.Cancel();
    cancel_token.Cancel();
    cancel_token.Cancel();

    wait_for_cancel->set_value(true);
    cancel_token.Cancel();
    ASSERT_TRUE(cancelled->load());
    ASSERT_EQ(std::future_status::ready,
              promise.get_future().wait_for(kCallbackWaitTime));

    future.wait();
    testing::Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE("Not merged");

    auto wait_for_cancel = std::make_shared<std::promise<bool>>();
    auto cancelled = std::make_shared<std::atomic_bool>(false);

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .WillOnce([&](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/) {
          auto current_request_id = request_id++;

          future = std::async(std::launch::async, [=]() {
            std::this_thread::sleep_for(kCallbackSleepTime);
            wait_for_cancel->get_future().get();
            callback(olp::http::NetworkResponse()
                         .WithStatus(http::HttpStatusCode::OK)
                         .WithRequestId(current_request_id));
          });

          return olp::http::SendOutcome(current_request_id);
        });

    EXPECT_CALL(*network, Cancel(request_id))
        .WillOnce([&](olp::http::RequestId) { cancelled->store(true); });

    std::promise<olp::client::HttpResponse> promise;
    auto callback = [&](olp::client::HttpResponse response) {
      promise.set_value(std::move(response));
    };

    const std::string content_string = "something";
    const auto content = std::make_shared<std::vector<unsigned char>>(
        content_string.begin(), content_string.end());

    auto cancel_token =
        client_.CallApi({}, "GET", {}, {}, {}, content, {}, callback);

    // Cancel multiple times
    cancel_token.Cancel();
    cancel_token.Cancel();
    cancel_token.Cancel();

    wait_for_cancel->set_value(true);
    cancel_token.Cancel();
    ASSERT_TRUE(cancelled->load());
    ASSERT_EQ(std::future_status::ready,
              promise.get_future().wait_for(kCallbackWaitTime));

    future.wait();
    testing::Mock::VerifyAndClearExpectations(network.get());
  }
}

TEST_P(OlpClientTest, CancelRetry) {
  auto network = network_;
  client_settings_.retry_settings.max_attempts = 6;
  client_settings_.retry_settings.initial_backdown_period = 500;
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse& response) {
        return response.status == http::HttpStatusCode::TOO_MANY_REQUESTS;
      });
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  {
    SCOPED_TRACE("Merged");

    auto cancel_wait = std::make_shared<std::promise<void>>();
    auto continue_network = std::make_shared<std::promise<void>>();
    auto cancelled = std::make_shared<std::atomic_bool>(false);
    auto retries = std::make_shared<int>(0);

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .WillRepeatedly(
            [&](olp::http::NetworkRequest /*request*/,
                olp::http::Network::Payload /*payload*/,
                olp::http::Network::Callback callback,
                olp::http::Network::HeaderCallback /*header_callback*/,
                olp::http::Network::DataCallback /*data_callback*/) {
              auto tries = ++(*retries);
              auto current_request_id = request_id++;

              future = std::async(std::launch::async, [=]() {
                std::this_thread::sleep_for(kCallbackSleepTime);
                if (tries == 1) {
                  cancel_wait->set_value();
                  continue_network->get_future().get();
                }

                callback(olp::http::NetworkResponse()
                             .WithStatus(http::HttpStatusCode::OK)
                             .WithRequestId(current_request_id));
              });

              return olp::http::SendOutcome(current_request_id);
            });

    EXPECT_CALL(*network, Cancel(request_id))
        .WillOnce([=](olp::http::RequestId) { cancelled->store(true); });

    olp::client::CancellationContext context;

    auto response = std::async(std::launch::async, [&]() {
      return call_wrapper_->CallApi({}, {}, {}, {}, {}, nullptr, {}, context);
    });

    cancel_wait->get_future().get();
    context.CancelOperation();
    continue_network->set_value();

    EXPECT_TRUE(cancelled->load());
    ASSERT_EQ(std::future_status::ready, response.wait_for(kCallbackWaitTime));
    auto response_value = response.get();
    ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
              response_value.GetStatus());
    ASSERT_LT(*retries, client_settings_.retry_settings.max_attempts);

    future.wait();
    testing::Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE("Not merged");

    auto cancel_wait = std::make_shared<std::promise<void>>();
    auto continue_network = std::make_shared<std::promise<void>>();
    auto cancelled = std::make_shared<std::atomic_bool>(false);
    auto retries = std::make_shared<int>(0);

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .WillRepeatedly(
            [&](olp::http::NetworkRequest /*request*/,
                olp::http::Network::Payload /*payload*/,
                olp::http::Network::Callback callback,
                olp::http::Network::HeaderCallback /*header_callback*/,
                olp::http::Network::DataCallback /*data_callback*/) {
              auto tries = ++(*retries);
              auto current_request_id = request_id++;

              future = std::async(std::launch::async, [=]() {
                std::this_thread::sleep_for(kCallbackSleepTime);
                if (tries == 1) {
                  cancel_wait->set_value();
                  continue_network->get_future().get();
                }

                callback(olp::http::NetworkResponse()
                             .WithStatus(http::HttpStatusCode::OK)
                             .WithRequestId(current_request_id));
              });

              return olp::http::SendOutcome(current_request_id);
            });

    EXPECT_CALL(*network, Cancel(request_id))
        .WillOnce([=](olp::http::RequestId) { cancelled->store(true); });

    olp::client::CancellationContext context;
    const std::string content_string = "something";
    const auto content = std::make_shared<std::vector<unsigned char>>(
        content_string.begin(), content_string.end());

    auto response = std::async(std::launch::async, [&]() {
      return call_wrapper_->CallApi({}, {}, {}, {}, {}, content, {}, context);
    });

    cancel_wait->get_future().get();
    context.CancelOperation();
    continue_network->set_value();

    EXPECT_TRUE(cancelled->load());
    ASSERT_EQ(std::future_status::ready, response.wait_for(kCallbackWaitTime));
    auto response_value = response.get();
    ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
              response_value.GetStatus());
    ASSERT_LT(*retries, client_settings_.retry_settings.max_attempts);

    future.wait();
    testing::Mock::VerifyAndClearExpectations(network.get());
  }
}

TEST_P(OlpClientTest, SlowDownError) {
  auto network = network_;
  client_settings_.retry_settings.max_attempts = 0;
  client_.SetSettings(client_settings_);
  constexpr int kExpectedError =
      static_cast<int>(http::ErrorCode::NETWORK_OVERLOAD_ERROR);

  {
    SCOPED_TRACE("Merged");

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .WillOnce([&](olp::http::NetworkRequest, olp::http::Network::Payload,
                      olp::http::Network::Callback,
                      olp::http::Network::HeaderCallback,
                      olp::http::Network::DataCallback) {
          return olp::http::SendOutcome(
              http::ErrorCode::NETWORK_OVERLOAD_ERROR);
        });

    auto response = call_wrapper_->CallApi({}, {}, {}, {}, {}, nullptr, {});

    EXPECT_EQ(kExpectedError, response.GetStatus());

    testing::Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Not merged");

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .WillOnce([&](olp::http::NetworkRequest, olp::http::Network::Payload,
                      olp::http::Network::Callback,
                      olp::http::Network::HeaderCallback,
                      olp::http::Network::DataCallback) {
          return olp::http::SendOutcome(
              http::ErrorCode::NETWORK_OVERLOAD_ERROR);
        });

    const std::string content_string = "something";
    const auto content = std::make_shared<std::vector<unsigned char>>(
        content_string.begin(), content_string.end());

    auto response = call_wrapper_->CallApi({}, {}, {}, {}, {}, content, {});

    EXPECT_EQ(kExpectedError, response.GetStatus());

    testing::Mock::VerifyAndClearExpectations(network.get());
  }
}

TEST_P(OlpClientTest, ApiKey) {
  std::string url;

  // Set OAuth2 provider and apiKey provider to be sure that apiKey provider
  // has more priority
  auto authentication_settings = olp::client::AuthenticationSettings();
  authentication_settings.api_key_provider = []() {
    return std::string("test-key");
  };

  authentication_settings.token_provider =
      [](olp::client::CancellationContext&) {
        return olp::client::OauthToken("secret",
                                       std::numeric_limits<time_t>::max());
      };

  auto network = network_;
  client_settings_.authentication_settings = authentication_settings;
  client_.SetSettings(client_settings_);

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload /*payload*/,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback /*data_callback*/) {
        EXPECT_EQ(request.GetUrl(), "here.com?apiKey=test-key");
        auto current_request_id = request_id++;

        future = std::async(std::launch::async, [=]() {
          std::this_thread::sleep_for(kCallbackSleepTime);
          callback(olp::http::NetworkResponse()
                       .WithStatus(http::HttpStatusCode::OK)
                       .WithRequestId(current_request_id));
        });

        return olp::http::SendOutcome(current_request_id);
      });

  auto response =
      call_wrapper_->CallApi("here.com", "GET", {}, {}, {}, nullptr, {});

  future.wait();
  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, TokenDeprecatedProvider) {
  {
    SCOPED_TRACE("EmptyBearer");
    // Make token provider generate empty strings. We expect no network requests
    // made in this case.
    auto authentication_settings = olp::client::AuthenticationSettings();
    authentication_settings.provider = []() { return std::string(""); };
    auto network = network_;
    client_settings_.authentication_settings = authentication_settings;
    client_.SetSettings(client_settings_);

    EXPECT_CALL(*network, Send(_, _, _, _, _)).Times(0);

    auto response =
        call_wrapper_->CallApi("here.com", "GET", {}, {}, {}, nullptr, {});
    EXPECT_EQ(response.GetStatus(),
              static_cast<int>(http::ErrorCode::AUTHORIZATION_ERROR));

    testing::Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Non empty token");
    std::string token("bearer-access-token");
    auto authentication_settings = olp::client::AuthenticationSettings();
    authentication_settings.provider = [token]() { return token; };
    auto network = network_;
    client_settings_.authentication_settings = authentication_settings;
    client_.SetSettings(client_settings_);

    olp::http::NetworkResponse response;
    response.WithStatus(olp::http::HttpStatusCode::OK);

    olp::http::NetworkRequest request("");
    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .WillOnce(testing::DoAll(testing::SaveArg<0>(&request),
                                 testing::InvokeArgument<2>(response),
                                 testing::Return(olp::http::SendOutcome(0))));

    auto api_response =
        call_wrapper_->CallApi("here.com", "GET", {}, {}, {}, nullptr, {});

    auto headers = request.GetHeaders();

    auto header_it = std::find_if(
        headers.begin(), headers.end(), [&](const olp::http::Header& header) {
          return header.first == olp::http::kAuthorizationHeader &&
                 header.second == "Bearer " + token;
        });

    EXPECT_NE(header_it, headers.end());

    testing::Mock::VerifyAndClearExpectations(network.get());
  }
}

TEST_P(OlpClientTest, EmptyBearerToken) {
  // Make token provider generate empty strings. We expect no network requests
  // made in this case.
  auto authentication_settings = olp::client::AuthenticationSettings();
  authentication_settings.token_provider =
      [](olp::client::CancellationContext&) {
        return olp::client::OauthToken("", std::chrono::seconds(5));
      };

  auto network = network_;
  client_settings_.authentication_settings = authentication_settings;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _)).Times(0);

  auto response =
      call_wrapper_->CallApi("here.com", "GET", {}, {}, {}, nullptr, {});
  EXPECT_EQ(response.GetStatus(),
            static_cast<int>(http::ErrorCode::AUTHORIZATION_ERROR));

  testing::Mock::VerifyAndClearExpectations(network.get());
}

TEST_P(OlpClientTest, ErrorOnTokenRequest) {
  // Make token provider generate an error. We expect no network requests made
  // in this case.
  auto authentication_settings = olp::client::AuthenticationSettings();
  authentication_settings.token_provider =
      [](olp::client::CancellationContext&) {
        return olp::client::ApiError(
            static_cast<int>(http::ErrorCode::NETWORK_OVERLOAD_ERROR),
            "Error message");
      };

  auto network = network_;
  client_settings_.authentication_settings = authentication_settings;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _)).Times(0);

  auto response =
      call_wrapper_->CallApi("here.com", "GET", {}, {}, {}, nullptr, {});
  EXPECT_EQ(response.GetStatus(),
            static_cast<int>(http::ErrorCode::NETWORK_OVERLOAD_ERROR));

  testing::Mock::VerifyAndClearExpectations(network.get());
}

INSTANTIATE_TEST_SUITE_P(, OlpClientTest,
                         ::testing::Values(CallApiType::ASYNC,
                                           CallApiType::SYNC));

class OlpClientMergeTest : public ::testing::Test {
 public:
  void SetUp() override {
    network_ = std::make_shared<testing::StrictMock<NetworkMock>>();
    settings_.network_request_handler = network_;
  }

  olp::client::OlpClientSettings settings_;
  olp::client::OlpClient client_;
  std::shared_ptr<testing::StrictMock<NetworkMock>> network_;
};

TEST_F(OlpClientMergeTest, MergeMultipleCallbacks) {
  const std::string path = "/layers/xyz/versions/1/quadkeys/23618402/depths/4";
  client_.SetBaseUrl(
      "https://api.platform.here.com/query/v1/catalogs/hrn:here:data:::dummy");
  auto network = network_;
  client_.SetSettings(settings_);
  constexpr size_t kExpectedCallbacks = 3u;

  std::future<void> future;
  olp::http::RequestId request_id = 5;

  {
    SCOPED_TRACE("None cancelled");

    auto wait_for_release = std::make_shared<std::promise<void>>();

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .WillOnce([&](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/) {
          auto current_request_id = request_id++;

          future = std::async(std::launch::async, [=]() {
            std::this_thread::sleep_for(kCallbackSleepTime);
            wait_for_release->get_future().get();

            *payload << "content";
            callback(olp::http::NetworkResponse()
                         .WithStatus(http::HttpStatusCode::OK)
                         .WithRequestId(current_request_id));
          });

          return olp::http::SendOutcome(current_request_id);
        });

    size_t index = 0;

    // Trigger calls with multiple callbacks and check that only
    // one Network request is made and all callbacks are called
    for (size_t idx = 0; idx < kExpectedCallbacks; ++idx) {
      client_.CallApi(path, "GET", {}, {}, {}, nullptr, "application/json",
                      [&](olp::client::HttpResponse response) {
                        ++index;
                        SCOPED_TRACE(testing::Message() << "index=" << index);
                        std::string response_payload;
                        response.GetResponse(response_payload);
                        ASSERT_EQ("content", response_payload);
                        ASSERT_EQ(http::HttpStatusCode::OK,
                                  response.GetStatus());
                      });
    }

    wait_for_release->set_value();
    future.wait();

    EXPECT_EQ(3u, index);
    testing::Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE("One cancelled");

    auto wait_for_release = std::make_shared<std::promise<void>>();

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .WillOnce([&](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/) {
          auto current_request_id = request_id++;

          future = std::async(std::launch::async, [=]() {
            std::this_thread::sleep_for(kCallbackSleepTime);
            wait_for_release->get_future().get();

            *payload << "content";
            callback(olp::http::NetworkResponse()
                         .WithStatus(http::HttpStatusCode::OK)
                         .WithRequestId(current_request_id));
          });

          return olp::http::SendOutcome(current_request_id);
        });

    size_t index = 0;

    // Trigger calls with multiple callbacks and check that only
    // one Network request is made and all callbacks are called
    for (size_t idx = 0; idx < kExpectedCallbacks - 1; ++idx) {
      client_.CallApi(path, "GET", {}, {}, {}, nullptr, "application/json",
                      [&](olp::client::HttpResponse response) {
                        ++index;
                        SCOPED_TRACE(testing::Message() << "index=" << index);
                        std::string response_payload;
                        response.GetResponse(response_payload);
                        ASSERT_EQ("content", response_payload);
                        ASSERT_EQ(http::HttpStatusCode::OK,
                                  response.GetStatus());
                      });
    }

    auto cancellation_token = client_.CallApi(
        path, "GET", {}, {}, {}, nullptr, "application/json",
        [&](olp::client::HttpResponse response) {
          ++index;
          SCOPED_TRACE(testing::Message() << "index=" << index);
          std::string response_payload;
          response.GetResponse(response_payload);
          ASSERT_NE("content", response_payload);
          ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
                    response.GetStatus());
        });

    cancellation_token.Cancel();
    wait_for_release->set_value();
    future.wait();

    EXPECT_EQ(3u, index);
    testing::Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE("All cancelled");

    auto wait_for_release = std::make_shared<std::promise<void>>();

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .WillOnce([&](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/) {
          auto current_request_id = request_id++;

          future = std::async(std::launch::async, [=]() {
            std::this_thread::sleep_for(kCallbackSleepTime);
            wait_for_release->get_future().get();

            *payload << "Operation cancelled";
            callback(olp::http::NetworkResponse()
                         .WithStatus(static_cast<int>(
                             olp::http::ErrorCode::CANCELLED_ERROR))
                         .WithRequestId(current_request_id));
          });

          return olp::http::SendOutcome(current_request_id);
        });

    size_t index = 0;
    size_t cancel_index = 0;

    EXPECT_CALL(*network, Cancel(request_id))
        .WillOnce([&](olp::http::RequestId) { ++cancel_index; });

    std::vector<olp::client::CancellationToken> tokens;
    for (size_t idx = 0; idx < kExpectedCallbacks; ++idx) {
      tokens.emplace_back(client_.CallApi(
          path, "GET", {}, {}, {}, nullptr, "application/json",
          [&](olp::client::HttpResponse response) {
            ++index;
            SCOPED_TRACE(testing::Message() << "index=" << index);
            ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
                      response.GetStatus());
          }));
    }

    // Cancel all requests
    std::for_each(
        tokens.begin(), tokens.end(),
        [](const olp::client::CancellationToken& token) { token.Cancel(); });

    wait_for_release->set_value();
    future.wait();

    EXPECT_EQ(1u, cancel_index);
    EXPECT_EQ(3u, index);
    testing::Mock::VerifyAndClearExpectations(network.get());
  }
}

TEST_F(OlpClientMergeTest, NoMergeMultipleCallbacks) {
  const std::string path = "/layers/xyz/versions/1/quadkeys/23618402/depths/4";
  client_.SetBaseUrl(
      "https://api.platform.here.com/query/v1/catalogs/hrn:here:data:::dummy");
  auto network = network_;
  client_.SetSettings(settings_);
  constexpr size_t kExpectedCallbacks = 3u;

  std::vector<std::future<void>> futures;
  std::map<http::RequestId, std::shared_ptr<std::promise<void>>> promise_map;
  const std::string content_string = "something";
  const auto content = std::make_shared<std::vector<unsigned char>>(
      content_string.begin(), content_string.end());

  auto release_and_wait = [&](size_t expected_calls, const size_t& calls) {
    // Release the async network responses
    for (const auto& promise : promise_map) {
      promise.second->set_value();
    }

    for (auto& future : futures) {
      future.wait();
    }

    EXPECT_EQ(expected_calls, calls);
    testing::Mock::VerifyAndClearExpectations(network.get());
  };

  {
    SCOPED_TRACE("None cancelled");

    olp::http::RequestId request_id = 5;

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .Times(kExpectedCallbacks)
        .WillRepeatedly(
            [&](olp::http::NetworkRequest /*request*/,
                olp::http::Network::Payload payload,
                olp::http::Network::Callback callback,
                olp::http::Network::HeaderCallback /*header_callback*/,
                olp::http::Network::DataCallback /*data_callback*/) {
              auto current_request_id = request_id++;

              futures.emplace_back(std::async(std::launch::async, [=]() {
                std::this_thread::sleep_for(kCallbackSleepTime);
                const auto& release = promise_map.at(current_request_id);
                release->get_future().get();

                *payload << "content, request_id=" << current_request_id;
                callback(olp::http::NetworkResponse()
                             .WithStatus(http::HttpStatusCode::OK)
                             .WithRequestId(current_request_id));
              }));

              return olp::http::SendOutcome(current_request_id);
            });

    size_t index = 0;
    for (size_t idx = 0; idx < kExpectedCallbacks; ++idx) {
      const auto current_request_id = request_id;
      promise_map[current_request_id] = std::make_shared<std::promise<void>>();

      client_.CallApi(
          path, "GET", {}, {}, {}, content, "application/json",
          [current_request_id, &index](olp::client::HttpResponse response) {
            ++index;
            SCOPED_TRACE(testing::Message() << "index=" << index);

            std::string expected_payload =
                "content, request_id=" + std::to_string(current_request_id);

            std::string response_payload;
            response.GetResponse(response_payload);
            ASSERT_EQ(expected_payload, response_payload);
            ASSERT_EQ(http::HttpStatusCode::OK, response.GetStatus());
          });
    }

    release_and_wait(3u, index);
  }

  futures.clear();
  promise_map.clear();

  {
    SCOPED_TRACE("One cancelled");

    olp::http::RequestId request_id = 5;

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .Times(kExpectedCallbacks)
        .WillRepeatedly(
            [&](olp::http::NetworkRequest /*request*/,
                olp::http::Network::Payload payload,
                olp::http::Network::Callback callback,
                olp::http::Network::HeaderCallback /*header_callback*/,
                olp::http::Network::DataCallback /*data_callback*/) {
              auto current_request_id = request_id++;

              futures.emplace_back(std::async(std::launch::async, [=]() {
                std::this_thread::sleep_for(kCallbackSleepTime);
                const auto& release = promise_map.at(current_request_id);
                release->get_future().get();

                *payload << "content, request_id=" << current_request_id;
                callback(olp::http::NetworkResponse()
                             .WithStatus(http::HttpStatusCode::OK)
                             .WithRequestId(current_request_id));
              }));

              return olp::http::SendOutcome(current_request_id);
            });

    EXPECT_CALL(*network, Cancel(request_id))
        .WillOnce([&](olp::http::RequestId) {});

    size_t index = 0;
    for (size_t idx = 0; idx < kExpectedCallbacks; ++idx) {
      const auto current_request_id = request_id;
      promise_map[current_request_id] = std::make_shared<std::promise<void>>();

      auto token = client_.CallApi(
          path, "GET", {}, {}, {}, content, "application/json",
          [=, &index](olp::client::HttpResponse response) {
            ++index;
            SCOPED_TRACE(testing::Message() << "index=" << index);

            std::string expected_payload =
                "content, request_id=" + std::to_string(current_request_id);

            if (idx == 0) {
              std::string response_payload;
              response.GetResponse(response_payload);
              ASSERT_NE(expected_payload, response_payload);
              ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
                        response.GetStatus());
            } else {
              std::string response_payload;
              response.GetResponse(response_payload);
              ASSERT_EQ(expected_payload, response_payload);
              ASSERT_EQ(http::HttpStatusCode::OK, response.GetStatus());
            }
          });

      // Cancel first call
      if (idx == 0) {
        token.Cancel();
      }
    }

    release_and_wait(3u, index);
  }

  futures.clear();
  promise_map.clear();

  {
    SCOPED_TRACE("All cancelled");

    olp::http::RequestId request_id = 5;
    const std::string expected_payload = "Operation cancelled";

    EXPECT_CALL(*network, Send(_, _, _, _, _))
        .Times(kExpectedCallbacks)
        .WillRepeatedly(
            [&](olp::http::NetworkRequest /*request*/,
                olp::http::Network::Payload payload,
                olp::http::Network::Callback callback,
                olp::http::Network::HeaderCallback /*header_callback*/,
                olp::http::Network::DataCallback /*data_callback*/) {
              auto current_request_id = request_id++;

              futures.emplace_back(std::async(std::launch::async, [=]() {
                std::this_thread::sleep_for(kCallbackSleepTime);
                const auto& release = promise_map.at(current_request_id);
                release->get_future().get();

                *payload << expected_payload;
                callback(olp::http::NetworkResponse()
                             .WithStatus(static_cast<int>(
                                 http::ErrorCode::CANCELLED_ERROR))
                             .WithRequestId(current_request_id));
              }));

              return olp::http::SendOutcome(current_request_id);
            });

    EXPECT_CALL(*network, Cancel(_))
        .Times(3)
        .WillRepeatedly([&](olp::http::RequestId) {});

    size_t index = 0;
    for (size_t idx = 0; idx < kExpectedCallbacks; ++idx) {
      const auto current_request_id = request_id;
      promise_map[current_request_id] = std::make_shared<std::promise<void>>();

      client_
          .CallApi(path, "GET", {}, {}, {}, content, "application/json",
                   [=, &index](olp::client::HttpResponse response) {
                     ++index;
                     SCOPED_TRACE(testing::Message() << "index=" << index);

                     std::string response_payload;
                     response.GetResponse(response_payload);
                     ASSERT_EQ(expected_payload, response_payload);
                     ASSERT_EQ(static_cast<int>(
                                   olp::http::ErrorCode::CANCELLED_ERROR),
                               response.GetStatus());
                   })
          .Cancel();
    }

    release_and_wait(3u, index);
  }
}

}  // namespace
