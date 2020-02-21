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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <queue>
#include <string>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/OlpClientSettingsFactory.h>

#include <olp/core/http/Network.h>
#include <olp/core/logging/Log.h>

namespace {
using olp::client::HttpResponse;
using ::testing::_;

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

class CallApiSync : public CallApiWrapper {
 public:
  explicit CallApiSync(const olp::client::OlpClient& client)
      : client_{client} {}

  HttpResponse CallApi(
      std::string path, std::string method,
      std::multimap<std::string, std::string> query_params,
      std::multimap<std::string, std::string> header_params,
      std::multimap<std::string, std::string> form_params,
      std::shared_ptr<std::vector<unsigned char>> post_body,
      std::string content_type,
      olp::client::CancellationContext context) const override {
    return client_.CallApi(std::move(path), std::move(method),
                           std::move(query_params), std::move(header_params),
                           std::move(form_params), std::move(post_body),
                           std::move(content_type), context);
  }

 private:
  const olp::client::OlpClient& client_;
};

class CallApiAsync : public CallApiWrapper {
 public:
  explicit CallApiAsync(const olp::client::OlpClient& client)
      : client_{client} {}

  HttpResponse CallApi(
      std::string path, std::string method,
      std::multimap<std::string, std::string> query_params,
      std::multimap<std::string, std::string> header_params,
      std::multimap<std::string, std::string> form_params,
      std::shared_ptr<std::vector<unsigned char>> post_body,
      std::string content_type,
      olp::client::CancellationContext context) const override {
    std::promise<olp::client::HttpResponse> promise;
    olp::client::NetworkAsyncCallback callback =
        [&promise](olp::client::HttpResponse http_response) {
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
  }

 private:
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

enum class CallApiType { ASYNC, SYNC };

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
    switch (GetParam()) {
      case CallApiType::ASYNC:
        call_wrapper_ = std::make_shared<CallApiAsync>(client_);
        break;
      case CallApiType::SYNC:
        call_wrapper_ = std::make_shared<CallApiSync>(client_);
        break;
      default:
        ADD_FAILURE() << "Invalid type of CallApi wrapper";
        break;
    }
  }

  olp::client::OlpClientSettings client_settings_;
  olp::client::OlpClient client_;
  std::shared_ptr<CallApiWrapper> call_wrapper_;
};

TEST_P(OlpClientTest, NumberOfAttempts) {
  client_settings_.retry_settings.max_attempts = 5;
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse&) { return true; });
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(6)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        callback(olp::http::NetworkResponse().WithStatus(429));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  client_.SetSettings(client_settings_);

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());

  ASSERT_EQ(429, response.status);
}

TEST_P(OlpClientTest, ZeroAttempts) {
  client_settings_.retry_settings.max_attempts = 0;
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse&) { return true; });

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        callback(olp::http::NetworkResponse().WithStatus(429));

        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  client_.SetSettings(client_settings_);

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());

  ASSERT_EQ(429, response.status);
}

TEST_P(OlpClientTest, DefaultRetryCondition) {
  client_settings_.retry_settings.max_attempts = 3;

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;

  auto attempt_statuses = std::queue<int>{{500, 503, 200}};
  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(static_cast<int>(attempt_statuses.size()))
      .WillRepeatedly([&attempt_statuses](
                          olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        if (attempt_statuses.empty()) {
          ADD_FAILURE_AT(__FILE__, __LINE__) << "Unexpected retry attempt";
          return olp::http::SendOutcome(olp::http::ErrorCode::UNKNOWN_ERROR);
        }

        auto status = attempt_statuses.front();
        attempt_statuses.pop();
        callback(olp::http::NetworkResponse().WithStatus(status));

        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  client_.SetSettings(client_settings_);

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());

  ASSERT_EQ(200, response.status);
}

TEST_P(OlpClientTest, RetryCondition) {
  client_settings_.retry_settings.max_attempts = 6;
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse& response) {
        return response.status == 429;
      });

  int current_attempt = 0;
  const int goodAttempt = 4;

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(goodAttempt)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        current_attempt++;
        if (current_attempt == goodAttempt) {
          callback(olp::http::NetworkResponse().WithStatus(200));
        } else {
          callback(olp::http::NetworkResponse().WithStatus(429));
        }
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  client_.SetSettings(client_settings_);

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());

  ASSERT_EQ(200, response.status);
}

TEST_P(OlpClientTest, RetryTimeLinear) {
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse&) { return true; });
  std::vector<std::chrono::time_point<std::chrono::system_clock>> timestamps;

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(4)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        timestamps.push_back(std::chrono::system_clock::now());
        callback(olp::http::NetworkResponse().WithStatus(429));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());

  ASSERT_EQ(1 + client_settings_.retry_settings.max_attempts,
            timestamps.size());
  ASSERT_EQ(429, response.status);
  for (size_t i = 1; i < timestamps.size(); ++i) {
    ASSERT_GE(timestamps.at(i) - timestamps.at(i - 1),
              std::chrono::milliseconds(
                  client_settings_.retry_settings.initial_backdown_period));
  }
}

TEST_P(OlpClientTest, RetryTimeExponential) {
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse&) { return true; });
  client_settings_.retry_settings.backdown_policy =
      ([](unsigned int milliseconds) { return 2 * milliseconds; });
  std::vector<std::chrono::time_point<std::chrono::system_clock>> timestamps;

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;

  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(4)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        timestamps.push_back(std::chrono::system_clock::now());
        callback(olp::http::NetworkResponse().WithStatus(429));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());

  ASSERT_EQ(1 + client_settings_.retry_settings.max_attempts,
            timestamps.size());
  ASSERT_EQ(429, response.status);
  auto backdownPeriod = client_settings_.retry_settings.initial_backdown_period;
  for (size_t i = 1; i < timestamps.size(); ++i) {
    ASSERT_GE(timestamps.at(i) - timestamps.at(i - 1),
              std::chrono::milliseconds(backdownPeriod));
    backdownPeriod *= 2;
  }
}

TEST_P(OlpClientTest, RetryWithExponentialBackdownStrategy) {
  const std::chrono::milliseconds::rep kInitialBackdownPeriod = 100;
  size_t expected_retry_count = 0;
  std::vector<std::chrono::milliseconds::rep> wait_times = {
      kInitialBackdownPeriod};

  // Setup retry settings:
  client_settings_.retry_settings.initial_backdown_period =
      kInitialBackdownPeriod;

  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse&) { return true; });

  client_settings_.retry_settings.backdown_strategy =
      [kInitialBackdownPeriod, &expected_retry_count, &wait_times](
          std::chrono::milliseconds period,
          size_t retry_count) -> std::chrono::milliseconds {
    EXPECT_EQ(kInitialBackdownPeriod, period.count());
    EXPECT_EQ(++expected_retry_count, retry_count);

    const auto wait_time = olp::client::ExponentialBackdownStrategy()(
        std::chrono::milliseconds(kInitialBackdownPeriod), retry_count);
    wait_times.push_back(wait_time.count());
    return wait_time;
  };

  // previous version of backdown policy shouldn't be called,
  // if the new backdown policy was specified
  client_settings_.retry_settings.backdown_policy = [](int period) -> int {
    ADD_FAILURE();
    return period;
  };

  auto network = std::make_shared<NetworkMock>();
  const auto requests_count = client_settings_.retry_settings.max_attempts + 1;
  std::vector<std::chrono::system_clock::time_point> timestamps;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(requests_count)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        timestamps.push_back(std::chrono::system_clock::now());
        callback(olp::http::NetworkResponse().WithStatus(
            olp::http::HttpStatusCode::TOO_MANY_REQUESTS));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);
  auto response = call_wrapper_->CallApi("", "GET", {}, {}, {}, nullptr, "");

  ASSERT_EQ(olp::http::HttpStatusCode::TOO_MANY_REQUESTS, response.status);
  ASSERT_EQ(client_settings_.retry_settings.max_attempts, expected_retry_count);

  // verify that duration between retries matches actual wait time from
  // backdown policy
  for (size_t i = 1; i < timestamps.size(); ++i) {
    ASSERT_GE((timestamps[i] - timestamps[i - 1]).count(), wait_times[i - 1]);
  }
}

TEST_P(OlpClientTest, RetryTimeout) {
  const size_t kMaxRetries = 3;
  // Setup retry settings:
  client_settings_.retry_settings.initial_backdown_period = 400;  // msec
  client_settings_.retry_settings.max_attempts = kMaxRetries;
  client_settings_.retry_settings.timeout = 1;  // seconds

  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse&) { return true; });
  client_settings_.retry_settings.backdown_policy =
      ([](int timeout) -> int { return 2 * timeout; });

  size_t current_attempt = 0;
  const size_t kSuccessfulAttempt = kMaxRetries + 1;

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        current_attempt++;
        // the test shouldn't reach the last retry due to timeout
        // restrictions in retry settings
        if (current_attempt == kSuccessfulAttempt) {
          ADD_FAILURE();

          callback(olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK));
        } else {
          callback(olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::TOO_MANY_REQUESTS));
        }
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);
  auto response = call_wrapper_->CallApi("", "GET", {}, {}, {}, nullptr, "");

  ASSERT_EQ(olp::http::HttpStatusCode::TOO_MANY_REQUESTS, response.status);
}

TEST_P(OlpClientTest, SetInitialBackdownPeriod) {
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse&) { return true; });
  client_settings_.retry_settings.initial_backdown_period = 1000;
  ASSERT_EQ(1000, client_settings_.retry_settings.initial_backdown_period);
  std::vector<std::chrono::time_point<std::chrono::system_clock>> timestamps;

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(4)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        timestamps.push_back(std::chrono::system_clock::now());
        callback(olp::http::NetworkResponse().WithStatus(429));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });
  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());

  ASSERT_EQ(1 + client_settings_.retry_settings.max_attempts,
            timestamps.size());
  ASSERT_EQ(429, response.status);
  for (size_t i = 1; i < timestamps.size(); ++i) {
    ASSERT_GE(timestamps.at(i) - timestamps.at(i - 1),
              std::chrono::milliseconds(
                  client_settings_.retry_settings.initial_backdown_period));
  }
}

TEST_P(OlpClientTest, Timeout) {
  client_settings_.retry_settings.timeout = 100;
  int timeout = 0;
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        timeout = request.GetSettings().GetConnectionTimeout();
        callback(olp::http::NetworkResponse().WithStatus(429));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());

  ASSERT_EQ(client_settings_.retry_settings.timeout, timeout);
  ASSERT_EQ(429, response.status);
}

TEST_P(OlpClientTest, Proxy) {
  client_settings_.retry_settings.timeout = 100;
  auto settings = olp::http::NetworkProxySettings()
                      .WithHostname("somewhere")
                      .WithPort(1080)
                      .WithType(olp::http::NetworkProxySettings::Type::HTTP)
                      .WithUsername("username1")
                      .WithPassword("1");

  client_settings_.proxy_settings = settings;
  auto expected_proxy_settings =
      olp::http::NetworkProxySettings()
          .WithHostname("somewhere")
          .WithPort(1080)
          .WithType(olp::http::NetworkProxySettings::Type::HTTP)
          .WithUsername("username1")
          .WithPassword("1");

  olp::http::NetworkProxySettings resultSettings;

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        resultSettings = request.GetSettings().GetProxySettings();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());

  ASSERT_EQ(expected_proxy_settings.GetHostname(),
            resultSettings.GetHostname());
  ASSERT_EQ(expected_proxy_settings.GetPassword(),
            resultSettings.GetPassword());
  ASSERT_EQ(expected_proxy_settings.GetPort(), resultSettings.GetPort());
  ASSERT_EQ(expected_proxy_settings.GetUsername(),
            resultSettings.GetUsername());
  ASSERT_EQ(expected_proxy_settings.GetPassword(),
            resultSettings.GetPassword());
}

TEST_P(OlpClientTest, EmptyProxy) {
  client_settings_.retry_settings.timeout = 100;

  auto settings = olp::http::NetworkProxySettings()
                      .WithHostname("somewhere")
                      .WithPort(1080)
                      .WithType(olp::http::NetworkProxySettings::Type::HTTP)
                      .WithUsername("username1")
                      .WithPassword("1");
  client_settings_.proxy_settings = settings;
  ASSERT_TRUE(client_settings_.proxy_settings);
  client_settings_.proxy_settings = boost::none;
  ASSERT_FALSE(client_settings_.proxy_settings);

  olp::http::NetworkProxySettings resultSettings;
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        resultSettings = request.GetSettings().GetProxySettings();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());

  ASSERT_FALSE(resultSettings.GetType() !=
               olp::http::NetworkProxySettings::Type::NONE);
}

TEST_P(OlpClientTest, HttpResponse) {
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        *payload << "content";
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());
  ASSERT_EQ("content", response.response.str());
  ASSERT_EQ(200, response.status);
}

TEST_P(OlpClientTest, Paths) {
  std::string url;
  client_.SetBaseUrl("here.com");
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        url = request.GetUrl();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  auto response = call_wrapper_->CallApi(
      "/index", "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());

  ASSERT_EQ("here.com/index", url);
}

TEST_P(OlpClientTest, MethodGET) {
  olp::http::NetworkRequest::HttpVerb verb;
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        verb = request.GetVerb();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());
  ASSERT_EQ(olp::http::NetworkRequest::HttpVerb::GET, verb);
}

TEST_P(OlpClientTest, MethodPOST) {
  olp::http::NetworkRequest::HttpVerb verb;
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        verb = request.GetVerb();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  auto response = call_wrapper_->CallApi(
      std::string(), "POST", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());
  ASSERT_EQ(olp::http::NetworkRequest::HttpVerb::POST, verb);
}

TEST_P(OlpClientTest, MethodPUT) {
  olp::http::NetworkRequest::HttpVerb verb;
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        verb = request.GetVerb();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(std::move(http_response));
      };

  auto response = call_wrapper_->CallApi(
      std::string(), "PUT", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());
  ASSERT_EQ(olp::http::NetworkRequest::HttpVerb::PUT, verb);
}

TEST_P(OlpClientTest, MethodDELETE) {
  olp::http::NetworkRequest::HttpVerb verb;
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        verb = request.GetVerb();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  auto response = call_wrapper_->CallApi(
      std::string(), "DELETE", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());
  ASSERT_EQ(olp::http::NetworkRequest::HttpVerb::DEL, verb);
}

TEST_P(OlpClientTest, QueryParam) {
  std::string url;
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        url = request.GetUrl();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  std::multimap<std::string, std::string> queryParams;
  queryParams.insert(std::make_pair("var1", ""));
  queryParams.insert(std::make_pair("var2", "2"));

  auto response = call_wrapper_->CallApi(
      "index", "GET", queryParams, std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());

  ASSERT_EQ("index?var1=&var2=2", url);
}

TEST_P(OlpClientTest, HeaderParams) {
  std::multimap<std::string, std::string> header_params;
  std::vector<std::pair<std::string, std::string>> result_headers;
  header_params.insert(std::make_pair("head1", "value1"));
  header_params.insert(std::make_pair("head2", "value2"));
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        result_headers = request.GetHeaders();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      header_params, std::multimap<std::string, std::string>(), nullptr,
      std::string());

  ASSERT_LE(2u, result_headers.size());
  for (auto& entry : result_headers) {
    if (entry.first == "head1") {
      ASSERT_EQ("value1", entry.second);
    } else if (entry.first == "head2") {
      ASSERT_EQ("value2", entry.second);
    }
  }
}

TEST_P(OlpClientTest, DefaultHeaderParams) {
  std::vector<std::pair<std::string, std::string>> result_headers;
  client_.GetMutableDefaultHeaders().insert(std::make_pair("head1", "value1"));
  client_.GetMutableDefaultHeaders().insert(std::make_pair("head2", "value2"));
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        result_headers = request.GetHeaders();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string());

  ASSERT_LE(2u, result_headers.size());
  for (auto& entry : result_headers) {
    if (entry.first == "head1") {
      ASSERT_EQ("value1", entry.second);
    } else if (entry.first == "head2") {
      ASSERT_EQ("value2", entry.second);
    }
  }
}

TEST_P(OlpClientTest, CombineHeaderParams) {
  std::vector<std::pair<std::string, std::string>> result_headers;
  client_.GetMutableDefaultHeaders().insert(std::make_pair("head1", "value1"));
  client_.GetMutableDefaultHeaders().insert(std::make_pair("head2", "value2"));
  std::multimap<std::string, std::string> header_params;
  header_params.insert(std::make_pair("head3", "value3"));
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        result_headers = request.GetHeaders();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      header_params, std::multimap<std::string, std::string>(), nullptr,
      std::string());

  ASSERT_LE(3u, result_headers.size());
  for (auto& entry : result_headers) {
    if (entry.first == "head1") {
      ASSERT_EQ("value1", entry.second);
    } else if (entry.first == "head2") {
      ASSERT_EQ("value2", entry.second);
    } else if (entry.first == "head3") {
      ASSERT_EQ("value3", entry.second);
    }
  }
}

TEST_P(OlpClientTest, Content) {
  std::vector<std::pair<std::string, std::string>> result_headers;
  client_.GetMutableDefaultHeaders().insert(std::make_pair("head1", "value1"));
  std::multimap<std::string, std::string> header_params;
  header_params.insert(std::make_pair("head3", "value3"));
  const std::string content_string = "something";
  const auto content = std::make_shared<std::vector<unsigned char>>(
      content_string.begin(), content_string.end());
  std::shared_ptr<const std::vector<unsigned char>> resultContent;

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        result_headers = request.GetHeaders();
        resultContent = request.GetBody();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });
  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(std::move(http_response));
      };

  auto response = call_wrapper_->CallApi(
      std::string(), "GET", std::multimap<std::string, std::string>(),
      header_params, std::multimap<std::string, std::string>(), content,
      "plain-text");
  ASSERT_LE(3u, result_headers.size());
  for (auto& entry : result_headers) {
    if (entry.first == "head1") {
      ASSERT_EQ("value1", entry.second);
    } else if (entry.first == "head3") {
      ASSERT_EQ("value3", entry.second);
    } else if (entry.first == "Content-Type") {
      ASSERT_EQ("plain-text", entry.second);
    }
  }
  ASSERT_TRUE(resultContent);
  ASSERT_EQ(*content, *resultContent);
}

TEST_P(OlpClientTest, CancelBeforeResponse) {
  auto wait_for_cancel = std::make_shared<std::promise<bool>>();
  auto wait_for_reach_network = std::make_shared<std::promise<bool>>();
  auto was_cancelled = std::make_shared<std::atomic_bool>(false);

  client_.SetBaseUrl("https://www.google.com");

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        std::thread handler_thread(
            [wait_for_reach_network, wait_for_cancel, callback]() {
              wait_for_reach_network->set_value(true);
              wait_for_cancel->get_future().get();
              callback(olp::http::NetworkResponse().WithStatus(200));
            });
        handler_thread.detach();

        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  EXPECT_CALL(*network, Cancel(5))
      .WillOnce(
          [=](olp::http::RequestId request_id) { was_cancelled->store(true); });

  olp::client::CancellationContext context;

  auto response_future = std::async(std::launch::async, [&]() {
    return call_wrapper_->CallApi(std::string(), "GET",
                                  std::multimap<std::string, std::string>(),
                                  std::multimap<std::string, std::string>(),
                                  std::multimap<std::string, std::string>(),
                                  nullptr, std::string(), context);
  });

  wait_for_reach_network->get_future().wait();
  context.CancelOperation();
  wait_for_cancel->set_value(true);
  ASSERT_EQ(std::future_status::ready,
            response_future.wait_for(std::chrono::seconds(2)));
  EXPECT_TRUE(was_cancelled->load());
}

TEST_P(OlpClientTest, CancelBeforeResponseAndCallHeadersCb) {
  auto wait_for_cancel = std::make_shared<std::promise<bool>>();
  auto wait_for_reach_network = std::make_shared<std::promise<bool>>();
  auto was_cancelled = std::make_shared<std::atomic_bool>(false);

  client_.SetBaseUrl("https://www.google.com");

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  olp::http::Network::HeaderCallback headers_cb;

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        std::thread handler_thread([wait_for_reach_network, wait_for_cancel,
                                    callback, header_callback, &headers_cb]() {
          headers_cb = std::move(header_callback);
          wait_for_reach_network->set_value(true);
          wait_for_cancel->get_future().get();
          callback(olp::http::NetworkResponse().WithStatus(200));
        });
        handler_thread.detach();

        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  EXPECT_CALL(*network, Cancel(5))
      .WillOnce(
          [=](olp::http::RequestId request_id) { was_cancelled->store(true); });

  olp::client::CancellationContext context;

  auto response_future = std::async(std::launch::async, [&]() {
    return call_wrapper_->CallApi(
        std::string(), "GET", std::multimap<std::string, std::string>(),
        {{"header", "header"}}, std::multimap<std::string, std::string>(),
        nullptr, std::string(), context);
  });
  wait_for_reach_network->get_future().wait();
  context.CancelOperation();
  wait_for_cancel->set_value(true);

  ASSERT_EQ(std::future_status::ready,
            response_future.wait_for(std::chrono::seconds(2)));

  // call headers callback after call was canceled
  headers_cb("header", "header");

  EXPECT_TRUE(was_cancelled->load());
}

// Test is valid only valid for sync api
TEST_P(OlpClientTest, CancelBeforeExecution) {
  client_.SetBaseUrl("https://www.google.com");
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _)).Times(0);
  olp::client::CancellationContext context;
  context.CancelOperation();
  auto response = client_.CallApi(std::string(), "GET",
                                  std::multimap<std::string, std::string>(),
                                  std::multimap<std::string, std::string>(),
                                  std::multimap<std::string, std::string>(),
                                  nullptr, std::string(), context);
  ASSERT_EQ(response.status,
            static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR));
}

// Test only make sense for async api, as CancellationContext::CancelOperation
TEST_P(OlpClientTest, CancelAfterCompletion) {
  auto was_cancelled = std::make_shared<std::atomic_bool>(false);

  client_.SetBaseUrl("https://www.google.com");
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  EXPECT_CALL(*network, Cancel(5))
      .WillOnce(
          [=](olp::http::RequestId request_id) { was_cancelled->store(true); });

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(std::move(http_response));
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  cancel_token.Cancel();

  ASSERT_TRUE(was_cancelled->load());
}

// Test only make sense for async api, as CancellationContext guards for
// double cancellation.
TEST_P(OlpClientTest, CancelDuplicate) {
  auto wait_for_cancel = std::make_shared<std::promise<bool>>();
  auto was_cancelled = std::make_shared<std::atomic_bool>(false);

  client_.SetBaseUrl("https://www.google.com");
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        std::thread handler_thread([wait_for_cancel, callback]() {
          wait_for_cancel->get_future().get();
          callback(olp::http::NetworkResponse().WithStatus(200));
        });
        handler_thread.detach();

        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  EXPECT_CALL(*network, Cancel(5))
      .WillOnce(
          [=](olp::http::RequestId request_id) { was_cancelled->store(true); });

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(std::move(http_response));
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  cancel_token.Cancel();
  cancel_token.Cancel();
  cancel_token.Cancel();
  wait_for_cancel->set_value(true);
  cancel_token.Cancel();
  ASSERT_TRUE(was_cancelled->load());
  ASSERT_EQ(std::future_status::ready,
            promise.get_future().wait_for(std::chrono::seconds(2)));
}

TEST_P(OlpClientTest, CancelRetry) {
  client_settings_.retry_settings.max_attempts = 6;
  client_settings_.retry_settings.initial_backdown_period = 500;
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse& response) {
        return response.status == 429;
      });

  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto continue_network = std::make_shared<std::promise<void>>();
  auto cancelled = std::make_shared<std::atomic_bool>(false);
  auto number_of_tries = std::make_shared<int>(0);

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        auto tries = ++(*number_of_tries);
        std::thread handler_thread(
            [continue_network, wait_for_cancel, callback, tries, cancelled]() {
              if (tries == 1) {
                wait_for_cancel->set_value();
                continue_network->get_future().get();
              }
              callback(olp::http::NetworkResponse().WithStatus(429));
            });
        handler_thread.detach();
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  olp::client::CancellationContext context;

  auto response = std::async(std::launch::async, [&]() {
    return call_wrapper_->CallApi(std::string(), std::string(),
                                  std::multimap<std::string, std::string>(),
                                  std::multimap<std::string, std::string>(),
                                  std::multimap<std::string, std::string>(),
                                  nullptr, std::string(), context);
  });

  wait_for_cancel->get_future().get();
  context.CancelOperation();
  continue_network->set_value();

  ASSERT_EQ(std::future_status::ready,
            response.wait_for(std::chrono::seconds(2)));
  auto response_value = response.get();
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            response_value.status)
      << response_value.response.str();
  ASSERT_LT(*number_of_tries, client_settings_.retry_settings.max_attempts);
}

TEST_P(OlpClientTest, QueryMultiParams) {
  std::string uri;
  std::vector<std::pair<std::string, std::string>> headers;
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest request,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback header_callback,
                    olp::http::Network::DataCallback data_callback) {
        uri = request.GetUrl();
        headers = request.GetHeaders();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  std::multimap<std::string, std::string> queryParams = {
      {"a", "a1"}, {"b", "b1"}, {"b", "b2"},
      {"c", "c1"}, {"c", "c2"}, {"c", "c3"}};

  std::multimap<std::string, std::string> header_params = {
      {"z", "z1"}, {"y", "y1"}, {"y", "y2"},
      {"x", "x1"}, {"x", "x2"}, {"x", "x3"}};

  std::multimap<std::string, std::string> form_params;

  auto response = call_wrapper_->CallApi(std::string(), std::string(),
                                         queryParams, header_params,
                                         form_params, nullptr, std::string());
  // query test
  for (auto q : queryParams) {
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
  ASSERT_FALSE(std::find_if(headers.begin(), headers.end(),
                            [new_value](decltype(new_value) el) {
                              return el == new_value;
                            }) != headers.end());
}

TEST_P(OlpClientTest, SlowDownError) {
  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest, olp::http::Network::Payload,
                    olp::http::Network::Callback,
                    olp::http::Network::HeaderCallback,
                    olp::http::Network::DataCallback) {
        return olp::http::SendOutcome(
            olp::http::ErrorCode::NETWORK_OVERLOAD_ERROR);
      });

  auto response = call_wrapper_->CallApi({}, {}, {}, {}, {}, nullptr, {});

  auto api_error =
      olp::client::ApiError{response.status, response.response.str()};
  EXPECT_EQ(olp::client::ErrorCode::SlowDown, api_error.GetErrorCode());
}

INSTANTIATE_TEST_SUITE_P(, OlpClientTest,
                         ::testing::Values(CallApiType::ASYNC,
                                           CallApiType::SYNC));
}  // namespace
