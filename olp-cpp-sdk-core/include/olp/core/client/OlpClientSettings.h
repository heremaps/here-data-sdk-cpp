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

#pragma once

#include <functional>
#include <memory>
#include <string>

#include <boost/optional.hpp>

#include "olp/core/client/CancellationToken.h"
#include "olp/core/http/Network.h"
#include "olp/core/network/NetworkProxy.h"

namespace olp {

namespace network {
class NetworkRequest;
class NetworkConfig;
class HttpResponse;
}  // namespace network

namespace thread {
class TaskScheduler;
}  // namespace thread

namespace client {

using NetworkAsyncCallback = std::function<void(network::HttpResponse)>;
using NetworkAsyncCancel = std::function<void()>;

/**
 * @brief Handler for asynchronous execution of network requests.
 */
using NetworkAsyncHandler = std::function<CancellationToken(
    const network::NetworkRequest&, const network::NetworkConfig&,
    const NetworkAsyncCallback&)>;

/**
 * @brief Default BackdownPolicy which simply returns the original wait time.
 */
CORE_API unsigned int DefaultBackdownPolicy(unsigned int milliseconds);

/**
 * @brief Default RetryCondition which is to disable retries.
 */
CORE_API bool DefaultRetryCondition(const network::HttpResponse& response);

struct AuthenticationSettings {
  /**
   * @brief Function to be implemented by the client which should return an
   * OAuth2 Bearer Access Token to be used as the Authorization Header on
   * Service calls.
   *
   * This allows for an external OAuth2 library to be used to provide
   * Authentication functionality for any Service.
   *
   * @note The provided token should be authorized to access the resources
   * provided by OLP Data Services you are trying to request. Also, the token
   * should not be expired by the time the Service request will be sent to the
   * server. Otherwise, a Service-specific Authorization error is returned when
   * calls are made.
   *
   * @note An empty string can be returned for the Token if the Service is
   * offline.
   */
  using TokenProviderCallback = std::function<std::string()>;

  /**
   * @brief A callback function which can be implemented by the client to allow
   * for cancelling an ongoing request to a TokenProviderCallback.
   *
   * This allows for a TokenProviderCallback request to be cancelled by calls to
   * a Service's cancel methods (e.g. CancellationToken). This method will only
   * be called if both it and a TokenProviderCallback callback are set and a
   * Service's cancel method is called while the TokenProviderCallback request
   * is currently being made (i.e. it will NOT be called if the
   * TokenProviderCallback request has finished and a Service's cancel method is
   * called while a subsequent part of the Service request is being made).
   * @see CancellationToken
   * @see TokenProviderCallback
   */
  using TokenProviderCancelCallback = std::function<void()>;

  TokenProviderCallback provider;
  boost::optional<TokenProviderCancelCallback> cancel;
};

struct RetrySettings {
  /**
   * @brief A function which takes the current timeout in milliseconds, and
   * returns what should be used for the next timeout. Allows an incremental
   * backdown or other strategy to be configured.
   */
  using BackdownPolicy = std::function<int(int)>;

  /**
   * @brief A function which in a HttpResponse, and a bool result. True if retry
   * is desired, false otherwise.
   */
  using RetryCondition = std::function<bool(const network::HttpResponse&)>;

  /**
   * @brief number of attempts. Default is 3.
   */
  int max_attempts = 3;

  /**
   * @brief The connection timeout limit (in seconds).
   */
  int timeout = 60;

  /**
   * @brief The period after an error, before the first retry attempt (in
   * milliseconds).
   */
  int initial_backdown_period = 200;

  /**
   * @brief The BackdownPolicy to use for retry attempts.
   * @return BackdownPolicy.
   */
  BackdownPolicy backdown_policy = DefaultBackdownPolicy;

  /**
   * @brief The RetryCondition used to evaluate responses to determine if retry
   * should be attempted.
   */
  RetryCondition retry_condition = DefaultRetryCondition;
};

/**
 * @brief OlpClient settings class. Use this class to configure the behaviour of
 * the OlpClient.
 */
struct OlpClientSettings {
  /**
   * @brief The retry settings to use.
   */
  RetrySettings retry_settings;

  /**
   * @brief The network proxy settings to use. Set boost::none to remove any
   * exisitng proxy settings.
   */
  boost::optional<network::NetworkProxy> proxy_settings = boost::none;

  /**
   * @brief The authentication settings to use. Set boost::none to remove
   * any existing settings.
   */
  boost::optional<AuthenticationSettings> authentication_settings = boost::none;

  /**
   * @brief The network handler.
   */
  boost::optional<NetworkAsyncHandler> network_async_handler = boost::none;

  /**
   * @brief The task scheduler instance. In case of nullptr set, all request
   * calls will be performed synchronous.
   */
  std::shared_ptr<thread::TaskScheduler> task_scheduler = nullptr;

  /**
   * @brief The network instance to be used to internally operate with OLP
   * services.
   */
  std::shared_ptr<http::Network> network_request_handler = nullptr;
};

}  // namespace client
}  // namespace olp
