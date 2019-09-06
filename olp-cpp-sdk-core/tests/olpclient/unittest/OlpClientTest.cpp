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
#include <string>

#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/OlpClientSettingsFactory.h>

#include <olp/core/http/Network.h>

using ::testing::_;

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

class Client : public ::testing::TestWithParam<bool> {
 protected:
  olp::client::OlpClientSettings client_settings_;
  olp::client::OlpClient client_;
};

INSTANTIATE_TEST_SUITE_P(TestOffline, Client, ::testing::Values(false));

class ClientDefaultAsyncHttp : public Client {};
INSTANTIATE_TEST_SUITE_P(TestOnline, ClientDefaultAsyncHttp,
                         ::testing::Values(true));

TEST_P(ClientDefaultAsyncHttp, GetGoogleWebsite) {
  client_.SetBaseUrl("https://www.google.com");

  client_settings_.network_request_handler = olp::client::
      OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();
  client_.SetSettings(client_settings_);

  std::promise<olp::client::HttpResponse> p;
  olp::client::NetworkAsyncCallback callback =
      [&p](olp::client::HttpResponse response) { p.set_value(response); };

  auto cancel_token = client_.CallApi(std::string(), std::string(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  auto response = p.get_future().get();
  ASSERT_EQ(200, response.status);
  ASSERT_LT(0u, response.response.size());
}

TEST_P(ClientDefaultAsyncHttp, GetNonExistentWebsite) {
  client_.SetBaseUrl("https://intranet.here212351.cococom");
  std::promise<olp::client::HttpResponse> p;
  olp::client::NetworkAsyncCallback callback =
      [&p](olp::client::HttpResponse response) { p.set_value(response); };

  client_settings_.network_request_handler = olp::client::
      OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();
  client_.SetSettings(client_settings_);

  auto cancel_token = client_.CallApi(std::string(), std::string(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);
  auto response = p.get_future().get();
  ASSERT_EQ(olp::http::ErrorCode::INVALID_URL_ERROR,
            static_cast<olp::http::ErrorCode>(response.status));
}

TEST_P(Client, numberOfAttempts) {
  client_settings_.retry_settings.max_attempts = 6;
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

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  ASSERT_EQ(429, response.status);
}

TEST_P(Client, ZeroAttempts) {
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

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  ASSERT_EQ(429, response.status);
}

TEST_P(Client, DefaultRetryCondition) {
  client_settings_.retry_settings.max_attempts = 6;

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

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  ASSERT_EQ(429, response.status);
}

TEST_P(Client, RetryCondition) {
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
      .WillRepeatedly([&current_attempt, goodAttempt](
                          olp::http::NetworkRequest request,
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

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  ASSERT_EQ(200, response.status);
}

TEST_P(Client, RetryTimeLinear) {
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse&) { return true; });
  std::vector<std::chrono::time_point<std::chrono::system_clock>> timestamps;

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(3)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        timestamps.push_back(std::chrono::system_clock::now());
        callback(olp::http::NetworkResponse().WithStatus(429));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  ASSERT_EQ(client_settings_.retry_settings.max_attempts, timestamps.size());
  ASSERT_EQ(429, response.status);
  for (size_t i = 1; i < timestamps.size(); ++i) {
    ASSERT_GE(timestamps.at(i) - timestamps.at(i - 1),
              std::chrono::milliseconds(
                  client_settings_.retry_settings.initial_backdown_period));
  }
}

TEST_P(Client, RetryTimeExponential) {
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse&) { return true; });
  client_settings_.retry_settings.backdown_policy =
      ([](unsigned int milliseconds) { return 2 * milliseconds; });
  std::vector<std::chrono::time_point<std::chrono::system_clock>> timestamps;

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;

  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(3)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        timestamps.push_back(std::chrono::system_clock::now());
        callback(olp::http::NetworkResponse().WithStatus(429));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  ASSERT_EQ(client_settings_.retry_settings.max_attempts, timestamps.size());
  ASSERT_EQ(429, response.status);
  auto backdownPeriod = client_settings_.retry_settings.initial_backdown_period;
  for (size_t i = 1; i < timestamps.size(); ++i) {
    ASSERT_GE(timestamps.at(i) - timestamps.at(i - 1),
              std::chrono::milliseconds(backdownPeriod));
    backdownPeriod *= 2;
  }
}

TEST_P(Client, SetInitialBackdownPeriod) {
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse&) { return true; });
  client_settings_.retry_settings.initial_backdown_period = 1000;
  ASSERT_EQ(1000, client_settings_.retry_settings.initial_backdown_period);
  std::vector<std::chrono::time_point<std::chrono::system_clock>> timestamps;

  auto network = std::make_shared<NetworkMock>();
  client_settings_.network_request_handler = network;
  client_.SetSettings(client_settings_);

  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(3)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        timestamps.push_back(std::chrono::system_clock::now());
        callback(olp::http::NetworkResponse().WithStatus(429));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  ASSERT_EQ(client_settings_.retry_settings.max_attempts, timestamps.size());
  ASSERT_EQ(429, response.status);
  for (size_t i = 1; i < timestamps.size(); ++i) {
    ASSERT_GE(timestamps.at(i) - timestamps.at(i - 1),
              std::chrono::milliseconds(
                  client_settings_.retry_settings.initial_backdown_period));
  }
}

TEST_P(Client, Timeout) {
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

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  auto response = promise.get_future().get();

  ASSERT_EQ(client_settings_.retry_settings.timeout, timeout);
  ASSERT_EQ(429, response.status);
}

TEST_P(Client, Proxy) {
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

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  client_.CallApi(std::string(), "GET",
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(), nullptr,
                  std::string(), callback);

  auto response = promise.get_future().get();

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

TEST_P(Client, EmptyProxy) {
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

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  client_.CallApi(std::string(), "GET",
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(), nullptr,
                  std::string(), callback);
  auto response = promise.get_future().get();

  ASSERT_FALSE(resultSettings.GetType() !=
               olp::http::NetworkProxySettings::Type::NONE);
}

TEST_P(Client, HttpResponse) {
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

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  auto response = promise.get_future().get();

  ASSERT_EQ("content", response.response);
  ASSERT_EQ(200, response.status);
}

TEST_P(Client, Paths) {
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

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  client_.CallApi("/index", "GET", std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(), nullptr,
                  std::string(), callback);

  auto response = promise.get_future().get();

  ASSERT_EQ("here.com/index", url);
}

TEST_P(Client, MethodGET) {
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
        promise.set_value(http_response);
      };

  client_.CallApi(std::string(), "GET",
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(), nullptr,
                  std::string(), callback);
  auto response = promise.get_future().get();
  ASSERT_EQ(olp::http::NetworkRequest::HttpVerb::GET, verb);
}

TEST_P(Client, MethodPOST) {
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
        promise.set_value(http_response);
      };

  client_.CallApi(std::string(), "POST",
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(), nullptr,
                  std::string(), callback);
  auto response = promise.get_future().get();
  ASSERT_EQ(olp::http::NetworkRequest::HttpVerb::POST, verb);
}

TEST_P(Client, MethodPUT) {
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
        promise.set_value(http_response);
      };

  client_.CallApi(std::string(), "PUT",
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(), nullptr,
                  std::string(), callback);
  auto response = promise.get_future().get();
  ASSERT_EQ(olp::http::NetworkRequest::HttpVerb::PUT, verb);
}

TEST_P(Client, MethodDELETE) {
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
        promise.set_value(http_response);
      };

  client_.CallApi(std::string(), "DELETE",
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(), nullptr,
                  std::string(), callback);
  auto response = promise.get_future().get();
  ASSERT_EQ(olp::http::NetworkRequest::HttpVerb::DEL, verb);
}

TEST_P(Client, QueryParam) {
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

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  client_.CallApi("index", "GET", queryParams,
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(), nullptr,
                  std::string(), callback);
  auto response = promise.get_future().get();
  ASSERT_EQ("index?var1=&var2=2", url);
}

TEST_P(Client, HeaderParams) {
  std::multimap<std::string, std::string> header_params;
  std::vector<std::pair<std::string, std::string>> resultHeaders;
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
        resultHeaders = request.GetHeaders();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  client_.CallApi(std::string(), "GET",
                  std::multimap<std::string, std::string>(), header_params,
                  std::multimap<std::string, std::string>(), nullptr,
                  std::string(), callback);
  auto response = promise.get_future().get();

  ASSERT_EQ(2u, resultHeaders.size());
  for (auto& entry : resultHeaders) {
    if (entry.first == "head1") {
      ASSERT_EQ("value1", entry.second);
    } else {
      ASSERT_EQ("value2", entry.second);
    }
  }
}

TEST_P(Client, DefaultHeaderParams) {
  std::vector<std::pair<std::string, std::string>> resultHeaders;
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
        resultHeaders = request.GetHeaders();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  client_.CallApi(std::string(), "GET",
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(),
                  std::multimap<std::string, std::string>(), nullptr,
                  std::string(), callback);
  auto response = promise.get_future().get();

  ASSERT_EQ(2u, resultHeaders.size());
  for (auto& entry : resultHeaders) {
    if (entry.first == "head1") {
      ASSERT_EQ("value1", entry.second);
    } else {
      ASSERT_EQ("value2", entry.second);
    }
  }
}

TEST_P(Client, CombineHeaderParams) {
  std::vector<std::pair<std::string, std::string>> resultHeaders;
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
        resultHeaders = request.GetHeaders();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  client_.CallApi(std::string(), "GET",
                  std::multimap<std::string, std::string>(), header_params,
                  std::multimap<std::string, std::string>(), nullptr,
                  std::string(), callback);
  auto response = promise.get_future().get();

  ASSERT_EQ(3u, resultHeaders.size());
  for (auto& entry : resultHeaders) {
    if (entry.first == "head1") {
      ASSERT_EQ("value1", entry.second);
    } else if (entry.first == "head2") {
      ASSERT_EQ("value2", entry.second);
    } else {
      ASSERT_EQ("value3", entry.second);
    }
  }
}

TEST_P(Client, Content) {
  std::vector<std::pair<std::string, std::string>> resultHeaders;
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
        resultHeaders = request.GetHeaders();
        resultContent = request.GetBody();
        callback(olp::http::NetworkResponse().WithStatus(200));
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });
  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  client_.CallApi(std::string(), "GET",
                  std::multimap<std::string, std::string>(), header_params,
                  std::multimap<std::string, std::string>(), content,
                  "plain-text", callback);
  auto response = promise.get_future().get();
  ASSERT_EQ(3u, resultHeaders.size());
  for (auto& entry : resultHeaders) {
    if (entry.first == "head1") {
      ASSERT_EQ("value1", entry.second);
    } else if (entry.first == "head3") {
      ASSERT_EQ("value3", entry.second);
    } else if (entry.first == "Content-Type") {
      ASSERT_EQ("plain-text", entry.second);
    } else {
      ASSERT_TRUE(false);
    }
  }
  ASSERT_TRUE(resultContent);
  ASSERT_EQ(*content, *resultContent);
}

TEST_P(Client, CancelBeforeResponse) {
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

  auto response_promise =
      std::make_shared<std::promise<olp::client::HttpResponse>>();
  olp::client::NetworkAsyncCallback callback =
      [response_promise](olp::client::HttpResponse http_response) {
        response_promise->set_value(http_response);
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  cancel_token.cancel();
  wait_for_cancel->set_value(true);
  ASSERT_TRUE(was_cancelled->load());
  ASSERT_EQ(std::future_status::ready,
            response_promise->get_future().wait_for(std::chrono::seconds(2)));
}

TEST_P(Client, CancelAfterCompletion) {
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
        promise.set_value(http_response);
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  cancel_token.cancel();

  ASSERT_TRUE(was_cancelled->load());
}

TEST_P(Client, CancelDuplicate) {
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

  auto response_promise =
      std::make_shared<std::promise<olp::client::HttpResponse>>();
  olp::client::NetworkAsyncCallback callback =
      [response_promise](olp::client::HttpResponse http_response) {
        response_promise->set_value(http_response);
      };

  auto cancel_token = client_.CallApi(std::string(), "GET",
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  cancel_token.cancel();
  cancel_token.cancel();
  cancel_token.cancel();
  wait_for_cancel->set_value(true);
  cancel_token.cancel();

  ASSERT_TRUE(was_cancelled->load());
  ASSERT_EQ(std::future_status::ready,
            response_promise->get_future().wait_for(std::chrono::seconds(2)));
}

TEST_P(Client, CancelRetry) {
  client_settings_.retry_settings.max_attempts = 6;
  client_settings_.retry_settings.initial_backdown_period = 500;
  client_settings_.retry_settings.retry_condition =
      ([](const olp::client::HttpResponse& response) {
        return response.status == 429;
      });

  auto wait_for_cancel = std::make_shared<std::promise<bool>>();
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
            [wait_for_cancel, callback, tries, cancelled]() {
              callback(olp::http::NetworkResponse().WithStatus(429));
              if (tries == 1) {
                wait_for_cancel->set_value(true);
              }
            });
        handler_thread.detach();
        return olp::http::SendOutcome(olp::http::RequestId(5));
      });

  EXPECT_CALL(*network, Cancel(5)).WillOnce([cancelled](olp::http::RequestId) {
    cancelled->store(true);
  });

  auto callback_promise =
      std::make_shared<std::promise<olp::client::HttpResponse>>();
  olp::client::NetworkAsyncCallback callback =
      [callback_promise](olp::client::HttpResponse http_response) {
        callback_promise->set_value(http_response);
      };

  auto cancel_token = client_.CallApi(std::string(), std::string(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      std::multimap<std::string, std::string>(),
                                      nullptr, std::string(), callback);

  wait_for_cancel->get_future().get();
  cancel_token.cancel();

  EXPECT_EQ(std::future_status::ready,
            callback_promise->get_future().wait_for(std::chrono::seconds(2)));
  ASSERT_LT(*number_of_tries, client_settings_.retry_settings.max_attempts);
}

TEST_P(Client, QueryMultiParams) {
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

  std::promise<olp::client::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&promise](olp::client::HttpResponse http_response) {
        promise.set_value(http_response);
      };

  std::multimap<std::string, std::string> queryParams = {
      {"a", "a1"}, {"b", "b1"}, {"b", "b2"},
      {"c", "c1"}, {"c", "c2"}, {"c", "c3"}};

  std::multimap<std::string, std::string> header_params = {
      {"z", "z1"}, {"y", "y1"}, {"y", "y2"},
      {"x", "x1"}, {"x", "x2"}, {"x", "x3"}};

  std::multimap<std::string, std::string> form_params;

  auto cancel_token =
      client_.CallApi(std::string(), std::string(), queryParams, header_params,
                      form_params, nullptr, std::string(), callback);

  auto response = promise.get_future().get();

  // query test
  for (auto q : queryParams) {
    std::string param_equal_value = q.first + "=" + q.second;
    ASSERT_NE(uri.find(param_equal_value), std::string::npos);
  }
  ASSERT_EQ(uri.find("not=present"), std::string::npos);

  // headers test
  ASSERT_EQ(headers.size(), 6);
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
