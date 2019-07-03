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

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <future>

#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>

#include <olp/core/network/HttpResponse.h>
#include <olp/core/network/Network.h>
#include <olp/core/network/NetworkConfig.h>
#include <olp/core/network/NetworkRequest.h>

#include "NetworkAsyncHandlerImpl.h"

// Mark tests with no need for connectivity as TestOffline, this is just for
// convenience of filtering
class Client : public ::testing::TestWithParam<bool> {
 protected:
  olp::client::OlpClientSettings m_clientSettings;
  olp::client::OlpClient m_client;
};
INSTANTIATE_TEST_SUITE_P(TestOffline, Client, ::testing::Values(false));

class ClientDefaultNetwork : public Client {};
INSTANTIATE_TEST_SUITE_P(TestOnline, ClientDefaultNetwork,
                         ::testing::Values(true));

class ClientDefaultAsyncNetwork : public Client {};
INSTANTIATE_TEST_SUITE_P(TestOnline, ClientDefaultAsyncNetwork,
                         ::testing::Values(true));

TEST_P(ClientDefaultAsyncNetwork, getGoogleWebsite) {
  m_client.SetBaseUrl("https://www.google.com");

  std::promise<olp::network::HttpResponse> p;
  olp::client::NetworkAsyncCallback callback =
      [&p](olp::network::HttpResponse response) { p.set_value(response); };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  auto response = p.get_future().get();
  ASSERT_EQ(200, response.status);
  ASSERT_LT(0u, response.response.size());
}

TEST_P(ClientDefaultAsyncNetwork, getNonExistentWebsite) {
  m_client.SetBaseUrl("https://intranet.here212351.com");
  std::promise<olp::network::HttpResponse> p;
  olp::client::NetworkAsyncCallback callback =
      [&p](olp::network::HttpResponse response) { p.set_value(response); };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);
  auto response = p.get_future().get();
  ASSERT_EQ(olp::network::Network::InvalidURLError, response.status);
}

TEST_P(Client, numberOfAttempts) {
  m_clientSettings.retry_settings.max_attempts = 6;
  m_clientSettings.retry_settings.retry_condition =
      ([](const olp::network::HttpResponse&) { return true; });
  int numberOfTries = 0;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest&,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        ++numberOfTries;
        olp::network::HttpResponse response;
        response.status = 429;
        callback(response);
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  ASSERT_EQ(m_clientSettings.retry_settings.max_attempts, numberOfTries);
  ASSERT_EQ(429, response.status);
}

TEST_P(Client, zeroAttempts) {
  m_clientSettings.retry_settings.max_attempts = 0;
  m_clientSettings.retry_settings.retry_condition =
      ([](const olp::network::HttpResponse&) { return true; });
  int numberOfTries = 0;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest&,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        ++numberOfTries;
        olp::network::HttpResponse response;
        response.status = 429;
        callback(response);
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  ASSERT_EQ(1, numberOfTries);
  ASSERT_EQ(429, response.status);
}

TEST_P(Client, defaultRetryCondition) {
  m_clientSettings.retry_settings.max_attempts = 6;
  int numberOfTries = 0;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest&,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        ++numberOfTries;
        olp::network::HttpResponse response;
        response.status = 429;
        callback(response);
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);
  auto response = promise.get_future().get();
  ASSERT_EQ(1, numberOfTries);
  ASSERT_EQ(429, response.status);
}

TEST_P(Client, retryCondition) {
  m_clientSettings.retry_settings.max_attempts = 6;
  m_clientSettings.retry_settings.retry_condition =
      ([](const olp::network::HttpResponse& response) {
        return response.status == 429;
      });

  int numberOfTries = 0;
  int goodAttempt = 4;

  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest&,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        ++numberOfTries;
        olp::network::HttpResponse response;
        if (numberOfTries == goodAttempt) {
          response.status = 200;
        } else {
          response.status = 429;
        }
        callback(response);
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  ASSERT_EQ(goodAttempt, numberOfTries);
  ASSERT_EQ(200, response.status);
}

TEST_P(Client, retryTimeLinear) {
  m_clientSettings.retry_settings.retry_condition =
      ([](const olp::network::HttpResponse&) { return true; });
  std::vector<std::chrono::time_point<std::chrono::system_clock>> timestamps;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest&,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        timestamps.push_back(std::chrono::system_clock::now());
        olp::network::HttpResponse response;
        response.status = 429;
        callback(response);
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  ASSERT_EQ(m_clientSettings.retry_settings.max_attempts, timestamps.size());
  ASSERT_EQ(429, response.status);
  for (size_t i = 1; i < timestamps.size(); ++i) {
    ASSERT_GE(timestamps.at(i) - timestamps.at(i - 1),
              std::chrono::milliseconds(
                  m_clientSettings.retry_settings.initial_backdown_period));
  }
}

TEST_P(Client, retryTimeExponential) {
  m_clientSettings.retry_settings.retry_condition =
      ([](const olp::network::HttpResponse&) { return true; });
  m_clientSettings.retry_settings.backdown_policy =
      ([](unsigned int milliseconds) { return 2 * milliseconds; });
  std::vector<std::chrono::time_point<std::chrono::system_clock>> timestamps;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest&,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        timestamps.push_back(std::chrono::system_clock::now());
        olp::network::HttpResponse response;
        response.status = 429;
        callback(response);
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  ASSERT_EQ(m_clientSettings.retry_settings.max_attempts, timestamps.size());
  ASSERT_EQ(429, response.status);
  auto backdownPeriod = m_clientSettings.retry_settings.initial_backdown_period;
  for (size_t i = 1; i < timestamps.size(); ++i) {
    ASSERT_GE(timestamps.at(i) - timestamps.at(i - 1),
              std::chrono::milliseconds(backdownPeriod));
    backdownPeriod *= 2;
  }
}

TEST_P(Client, setInitialBackdownPeriod) {
  m_clientSettings.retry_settings.retry_condition =
      ([](const olp::network::HttpResponse&) { return true; });
  m_clientSettings.retry_settings.initial_backdown_period = 1000;
  ASSERT_EQ(1000, m_clientSettings.retry_settings.initial_backdown_period);
  std::vector<std::chrono::time_point<std::chrono::system_clock>> timestamps;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest&,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        timestamps.push_back(std::chrono::system_clock::now());
        olp::network::HttpResponse response;
        response.status = 429;
        callback(response);
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  ASSERT_EQ(m_clientSettings.retry_settings.max_attempts, timestamps.size());
  ASSERT_EQ(429, response.status);
  for (size_t i = 1; i < timestamps.size(); ++i) {
    ASSERT_GE(timestamps.at(i) - timestamps.at(i - 1),
              std::chrono::milliseconds(
                  m_clientSettings.retry_settings.initial_backdown_period));
  }
}

TEST_P(Client, timeout) {
  m_clientSettings.retry_settings.timeout = 100;
  int timeout = 0;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest&,
          const olp::network::NetworkConfig& config,
          const olp::client::NetworkAsyncCallback& callback) {
        timeout = config.ConnectTimeout();
        olp::network::HttpResponse response;
        response.status = 429;
        callback(response);
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  auto response = promise.get_future().get();

  ASSERT_EQ(m_clientSettings.retry_settings.timeout, timeout);
  ASSERT_EQ(429, response.status);
}

TEST_P(Client, proxy) {
  m_clientSettings.retry_settings.timeout = 100;
  olp::network::NetworkProxy settings("somewhere", 1080,
                                      olp::network::NetworkProxy::Type::Http,
                                      "username1", "1");
  m_clientSettings.proxy_settings = settings;
  olp::network::NetworkProxy resultSettings;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest&,
          const olp::network::NetworkConfig& config,
          const olp::client::NetworkAsyncCallback& callback) {
        resultSettings = config.Proxy();
        olp::network::HttpResponse response;
        callback(response);
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  auto response = promise.get_future().get();

  ASSERT_TRUE(resultSettings.IsValid());
  ASSERT_EQ(settings.Port(), resultSettings.Port());
  ASSERT_EQ(settings.UserName(), resultSettings.UserName());
  ASSERT_EQ(settings.UserPassword(), resultSettings.UserPassword());
  ASSERT_EQ(settings.Name(), resultSettings.Name());
}

TEST_P(Client, emptyProxy) {
  m_clientSettings.retry_settings.timeout = 100;

  olp::network::NetworkProxy settings("somewhere", 1080,
                                      olp::network::NetworkProxy::Type::Http,
                                      "username1", "1");
  m_clientSettings.proxy_settings = settings;
  ASSERT_TRUE(m_clientSettings.proxy_settings);
  m_clientSettings.proxy_settings = boost::none;
  ASSERT_FALSE(m_clientSettings.proxy_settings);

  olp::network::NetworkProxy resultSettings;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest&,
          const olp::network::NetworkConfig& config,
          const olp::client::NetworkAsyncCallback& callback) {
        resultSettings = config.Proxy();
        olp::network::HttpResponse response;
        callback(response);
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  auto response = promise.get_future().get();

  ASSERT_FALSE(resultSettings.IsValid());
}

TEST_P(Client, httpresponse) {
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest&,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        olp::network::HttpResponse response;
        response.status = 200;
        response.response = "content";
        callback(response);
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  auto response = promise.get_future().get();

  ASSERT_EQ("content", response.response);
  ASSERT_EQ(200, response.status);
}

TEST_P(Client, paths) {
  std::string url;
  m_client.SetBaseUrl("here.com");
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest& request,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        url = request.Url();
        callback(olp::network::HttpResponse());
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  m_client.CallApi(
      "/index", std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  auto response = promise.get_future().get();

  ASSERT_EQ("here.com/index", url);
}

TEST_P(Client, methodGET) {
  olp::network::NetworkRequest::HttpVerb verb;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest& request,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        verb = request.Verb();
        callback(olp::network::HttpResponse());
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  m_client.CallApi(std::string(), "GET", std::multimap<std::string, std::string>(),
                   std::multimap<std::string, std::string>(),
                   std::multimap<std::string, std::string>(), nullptr, std::string(),
                   callback);
  auto response = promise.get_future().get();
  ASSERT_EQ(olp::network::NetworkRequest::GET, verb);
}

TEST_P(Client, methodPUT) {
  olp::network::NetworkRequest::HttpVerb verb;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest& request,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        verb = request.Verb();
        callback(olp::network::HttpResponse());
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  m_client.CallApi(std::string(), "PUT", std::multimap<std::string, std::string>(),
                   std::multimap<std::string, std::string>(),
                   std::multimap<std::string, std::string>(), nullptr, std::string(),
                   callback);
  auto response = promise.get_future().get();
  ASSERT_EQ(olp::network::NetworkRequest::PUT, verb);
}

TEST_P(Client, methodDELETE) {
  olp::network::NetworkRequest::HttpVerb verb;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest& request,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        verb = request.Verb();
        callback(olp::network::HttpResponse());
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  m_client.CallApi(
      std::string(), "DELETE", std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);
  auto response = promise.get_future().get();
  ASSERT_EQ(olp::network::NetworkRequest::DEL, verb);
}

TEST_P(Client, methodPOST) {
  olp::network::NetworkRequest::HttpVerb verb;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest& request,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        verb = request.Verb();
        callback(olp::network::HttpResponse());
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  m_client.CallApi(std::string(), "POST", std::multimap<std::string, std::string>(),
                   std::multimap<std::string, std::string>(),
                   std::multimap<std::string, std::string>(), nullptr, std::string(),
                   callback);
  auto response = promise.get_future().get();
  ASSERT_EQ(olp::network::NetworkRequest::POST, verb);
}

TEST_P(Client, queryParam) {
  std::string url;
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest& request,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        url = request.Url();
        callback(olp::network::HttpResponse());
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);
  std::multimap<std::string, std::string> queryParams;
  queryParams.insert(std::make_pair("var1", ""));
  queryParams.insert(std::make_pair("var2", "2"));

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  m_client.CallApi(
      "index", std::string(), queryParams, std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);
  auto response = promise.get_future().get();
  ASSERT_EQ("index?var1=&var2=2", url);
}

TEST_P(Client, headerParams) {
  std::multimap<std::string, std::string> headerParams;
  std::vector<std::pair<std::string, std::string>> resultHeaders;
  headerParams.insert(std::make_pair("head1", "value1"));
  headerParams.insert(std::make_pair("head2", "value2"));
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest& request,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        resultHeaders = request.ExtraHeaders();
        callback(olp::network::HttpResponse());
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  m_client.CallApi(std::string(), std::string(),
                   std::multimap<std::string, std::string>(), headerParams,
                   std::multimap<std::string, std::string>(), nullptr, std::string(),
                   callback);
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

TEST_P(Client, defaultHeaderParams) {
  std::vector<std::pair<std::string, std::string>> resultHeaders;
  m_client.GetMutableDefaultHeaders().insert(std::make_pair("head1", "value1"));
  m_client.GetMutableDefaultHeaders().insert(std::make_pair("head2", "value2"));
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest& request,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        resultHeaders = request.ExtraHeaders();
        callback(olp::network::HttpResponse());
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);
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

TEST_P(Client, combineHeaderParams) {
  std::vector<std::pair<std::string, std::string>> resultHeaders;
  m_client.GetMutableDefaultHeaders().insert(std::make_pair("head1", "value1"));
  m_client.GetMutableDefaultHeaders().insert(std::make_pair("head2", "value2"));
  std::multimap<std::string, std::string> headerParams;
  headerParams.insert(std::make_pair("head3", "value3"));
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest& request,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        resultHeaders = request.ExtraHeaders();
        callback(olp::network::HttpResponse());
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  m_client.CallApi(std::string(), std::string(),
                   std::multimap<std::string, std::string>(), headerParams,
                   std::multimap<std::string, std::string>(), nullptr, std::string(),
                   callback);
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

TEST_P(Client, content) {
  std::vector<std::pair<std::string, std::string>> resultHeaders;
  m_client.GetMutableDefaultHeaders().insert(std::make_pair("head1", "value1"));
  std::multimap<std::string, std::string> headerParams;
  headerParams.insert(std::make_pair("head3", "value3"));
  const std::string content_string = "something";
  const auto content = std::make_shared<std::vector<unsigned char>>(
      content_string.begin(), content_string.end());
  std::shared_ptr<std::vector<unsigned char>> resultContent;

  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest& request,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        resultHeaders = request.ExtraHeaders();
        resultContent = request.Content();
        callback(olp::network::HttpResponse());
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  m_client.CallApi(std::string(), std::string(),
                   std::multimap<std::string, std::string>(), headerParams,
                   std::multimap<std::string, std::string>(), content, "plain-text",
                   callback);
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

TEST_P(Client, cancelBeforeResponse) {
  auto waitForCancel = std::make_shared<std::promise<bool>>();
  auto wasCancelled = std::make_shared<std::atomic_bool>(false);

  m_client.SetBaseUrl("https://www.google.com");
  m_clientSettings.network_async_handler =
      [waitForCancel, wasCancelled](
          const olp::network::NetworkRequest& /*request*/,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        // TODO maybe handler calls should be automatically off-threaded.
        std::thread handlerThread([waitForCancel, callback]() {
          waitForCancel->get_future().get();
          callback(olp::network::HttpResponse());
        });
        handlerThread.detach();
        return olp::client::CancellationToken(
            [wasCancelled]() { wasCancelled->store(true); });
      };
  m_client.SetSettings(m_clientSettings);

  auto responsePromise =
      std::make_shared<std::promise<olp::network::HttpResponse>>();
  olp::client::NetworkAsyncCallback callback =
      [responsePromise](olp::network::HttpResponse httpResponse) {
        responsePromise->set_value(httpResponse);
      };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  cancelFn.cancel();
  waitForCancel->set_value(true);
  ASSERT_TRUE(wasCancelled->load());
  ASSERT_EQ(std::future_status::ready,
            responsePromise->get_future().wait_for(std::chrono::seconds(2)));
}

TEST_P(Client, cancelAfterCompletion) {
  auto wasCancelled = std::make_shared<std::atomic_bool>(false);

  m_client.SetBaseUrl("https://www.google.com");
  m_clientSettings.network_async_handler =
      [&](const olp::network::NetworkRequest& /*request*/,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        callback(olp::network::HttpResponse());
        return olp::client::CancellationToken(
            [wasCancelled]() { wasCancelled->store(true); });
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  auto response = promise.get_future().get();
  cancelFn.cancel();

  ASSERT_TRUE(wasCancelled->load());
}

TEST_P(Client, cancelDuplicate) {
  auto waitForCancel = std::make_shared<std::promise<bool>>();
  auto wasCancelled = std::make_shared<std::atomic_bool>(false);

  m_client.SetBaseUrl("https://www.google.com");
  m_clientSettings.network_async_handler =
      [waitForCancel, wasCancelled](
          const olp::network::NetworkRequest& /*request*/,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        // TODO maybe handler calls should be automatically off-threaded.
        std::thread handlerThread([waitForCancel, callback]() {
          waitForCancel->get_future().get();
          callback(olp::network::HttpResponse());
        });
        handlerThread.detach();
        return olp::client::CancellationToken(
            [wasCancelled]() { wasCancelled->store(true); });
      };
  m_client.SetSettings(m_clientSettings);

  auto responsePromise =
      std::make_shared<std::promise<olp::network::HttpResponse>>();
  olp::client::NetworkAsyncCallback callback =
      [responsePromise](olp::network::HttpResponse httpResponse) {
        responsePromise->set_value(httpResponse);
      };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  cancelFn.cancel();
  cancelFn.cancel();
  cancelFn.cancel();
  waitForCancel->set_value(true);
  cancelFn.cancel();
  ASSERT_TRUE(wasCancelled->load());
  ASSERT_EQ(std::future_status::ready,
            responsePromise->get_future().wait_for(std::chrono::seconds(2)));
}

TEST_P(Client, cancelRetry) {
  m_clientSettings.retry_settings.max_attempts = 6;
  m_clientSettings.retry_settings.initial_backdown_period = 500;
  m_clientSettings.retry_settings.retry_condition =
      ([](const olp::network::HttpResponse& response) {
        return response.status == 429;
      });

  auto waitForCancel = std::make_shared<std::promise<bool>>();
  auto cancelled = std::make_shared<std::atomic_bool>(false);
  auto numberOfTries = std::make_shared<int>(0);
  m_clientSettings.network_async_handler =
      [waitForCancel, numberOfTries, cancelled](
          const olp::network::NetworkRequest&,
          const olp::network::NetworkConfig&,
          const olp::client::NetworkAsyncCallback& callback) {
        auto tries = ++(*numberOfTries);
        std::thread handlerThread(
            [waitForCancel, callback, tries, cancelled]() {
              olp::network::HttpResponse response;
              response.status = 429;
              callback(response);

              if (tries == 1) {
                waitForCancel->set_value(true);
              }
            });
        handlerThread.detach();

        return olp::client::CancellationToken(
            [cancelled]() { cancelled->store(true); });
      };
  m_client.SetSettings(m_clientSettings);

  auto callbackPromise =
      std::make_shared<std::promise<olp::network::HttpResponse>>();
  olp::client::NetworkAsyncCallback callback =
      [callbackPromise](olp::network::HttpResponse httpResponse) {
        callbackPromise->set_value(httpResponse);
      };

  auto cancelFn = m_client.CallApi(
      std::string(), std::string(), std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(),
      std::multimap<std::string, std::string>(), nullptr, std::string(), callback);

  waitForCancel->get_future().get();
  cancelFn.cancel();

  ASSERT_EQ(std::future_status::ready,
            callbackPromise->get_future().wait_for(std::chrono::seconds(2)));
  ASSERT_LT(*numberOfTries, m_clientSettings.retry_settings.max_attempts);
}

TEST_P(Client, queryMultiParams) {
  std::string uri;
  std::vector<std::pair<std::string,std::string>> headers;
  m_clientSettings.network_async_handler =
      [&uri, &headers](const olp::network::NetworkRequest& request,
          const olp::network::NetworkConfig& /*config*/,
          const olp::client::NetworkAsyncCallback& callback) {
        uri = request.Url();
        headers = request.ExtraHeaders();
        olp::network::HttpResponse response;
        callback(response);
        return olp::client::CancellationToken();
      };
  m_client.SetSettings(m_clientSettings);

  std::promise<olp::network::HttpResponse> promise;
  olp::client::NetworkAsyncCallback callback =
      [&](olp::network::HttpResponse httpResponse) {
        promise.set_value(httpResponse);
      };

  std::multimap<std::string, std::string> queryParams = {{"a", "a1"},
                                                         {"b", "b1"},
                                                         {"b", "b2"},
                                                         {"c", "c1"},
                                                         {"c", "c2"},
                                                         {"c", "c3"}};

  std::multimap<std::string, std::string> headerParams = {{"z", "z1"},
                                                          {"y", "y1"},
                                                          {"y", "y2"},
                                                          {"x", "x1"},
                                                          {"x", "x2"},
                                                          {"x", "x3"}};

  std::multimap<std::string, std::string> formParams;

  auto cancelFn = m_client.CallApi(std::string(), std::string(),
      queryParams, headerParams, formParams,
      nullptr, std::string(), callback);

  auto response = promise.get_future().get();

  // query test
  for(auto q : queryParams) {
      std::string paramEqualValue = q.first + "=" + q.second;
      ASSERT_NE(uri.find(paramEqualValue), std::string::npos);
  }
  ASSERT_EQ(uri.find("not=present"), std::string::npos);

  // headers test
  ASSERT_EQ(headers.size(), 6);
  for(auto p : headerParams) {
      ASSERT_TRUE(std::find_if(headers.begin(), headers.end(),
                               [p](decltype(p) el) {
                      return el == p;
                  }) != headers.end());
  }

  decltype(headerParams)::value_type newVal{"added", "new"};
  headerParams.insert(newVal);
  ASSERT_FALSE(std::find_if(headers.begin(), headers.end(),
                           [newVal](decltype(newVal) el) {
                  return el == newVal;
              }) != headers.end());
}
