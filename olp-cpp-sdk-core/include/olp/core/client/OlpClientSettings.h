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

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include <boost/optional.hpp>

#include <olp/core/client/BackdownStrategy.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HttpResponse.h>
#include <olp/core/http/Network.h>

namespace olp {
namespace cache {
class KeyValueCache;
}  // namespace cache

namespace thread {
class TaskScheduler;
}  // namespace thread

namespace http {
class Network;
}  // namespace http

namespace client {
/**
 * @brief The type alias of the asynchronous network callback.
 *
 * Used to receive the `HttpResponse` instance.
 */
using NetworkAsyncCallback = std::function<void(HttpResponse)>;

/**
 * @brief The type alias of the cancel function.
 *
 * Used to cancel the asynchronous network operation.
 */
using NetworkAsyncCancel = std::function<void()>;

/**
 * @brief The default backdown policy that returns the original wait time.
 */
CORE_API unsigned int DefaultBackdownPolicy(unsigned int milliseconds);

/**
 * @brief The default retry condition that disables retries.
 */
CORE_API bool DefaultRetryCondition(const olp::client::HttpResponse& response);

/**
 * @brief A set of settings that manages the `TokenProviderCallback` and
 * `TokenProviderCancelCallback` functions.
 *
 * The `TokenProviderCallback` function requests the OAuth2
 * bearer access token. The `TokenProviderCancelCallback` function cancels that
 * request. Both functions are user-provided.
 * The struct is used internally by the `OlpClient` class.
 */
struct CORE_API AuthenticationSettings {
  /**
   * @brief Implemented by the client that should return the OAuth2 bearer
   * access token.
   *
   * The access token should be used as the authorization header for the service
   * calls. This allows for an external OAuth2 library to be used to provide
   * the authentication functionality for any service.
   *
   * The provided token should be authorized to access the resources
   * provided by the HERE platform Services you are trying
   * to request. Also, the token should not be expired by the time the service
   * request is sent to the server. Otherwise, a service-specific authorization
   * error is returned when calls are made.
   *
   * An empty string can be returned for the token if the service is
   * offline.
   */
  using TokenProviderCallback = std::function<std::string()>;

  /**
   * @brief Cancels the ongoing `TokenProviderCallback` request.
   *
   * Cancels the `TokenProviderCallback` request by calls to the service cancel
   * methods (for example, `CancellationToken`). This method is only called if
   * both it and the `TokenProviderCallback` callback are set, and a service
   * cancel method is called while the `TokenProviderCallback` request is
   * currently being made. For example, it is not called if the
   * `TokenProviderCallback` request has finished and a service cancel method is
   * called while a subsequent part of the service request is being made.
   *
   * @see `CancellationToken` and `TokenProviderCallback` or more
   * details.
   */
  using TokenProviderCancelCallback = std::function<void()>;

  /**
   * @brief The user-provided function that returns the OAuth2 bearer access
   * token.
   *
   * @see `TokenProviderCallback` for more details.
   */
  TokenProviderCallback provider;

  /**
   * @brief (Optional) The user-provided function that is used to cancel
   * the ongoing access token request.
   *
   * @see `TokenProviderCallback` and `TokenProviderCancelCallback` for more
   * details.
   */
  boost::optional<TokenProviderCancelCallback> cancel;
};

/**
 * @brief A collection of settings that controls how failed requests should be
 * treated by the Data SDK.
 *
 * For example, it specifies whether the failed request should be retried, how
 * long Data SDK needs to wait for the next retry attempt, the number of maximum
 * retries, and so on.
 *
 * You can customize all of these settings. The settings are used internally by
 * the `OlpClient` class.
 */
struct CORE_API RetrySettings {
  /**
   * @brief Takes the current timeout (in milliseconds) and
   * returns what should be used for the next timeout.
   *
   * Allows an incremental backdown or other strategies to be configured.
   */
  using BackdownPolicy = std::function<int(int)>;

  /**
   * @brief Calculates the number of retry timeouts based on
   * the initial backdown duration and retries count.
   */
  using BackdownStrategy = std::function<std::chrono::milliseconds(
      std::chrono::milliseconds, size_t)>;

  /**
   * @brief Checks whether the retry is desired.
   *
   * @see `HttpResponse` for more details.
   */
  using RetryCondition = std::function<bool(const HttpResponse&)>;

  /**
   * @brief The number of attempts.
   *
   * The default value is 3.
   */
  int max_attempts = 3;

  /**
   * @brief The connection timeout limit (in seconds).
   */
  int timeout = 60;

  /**
   * @brief The period between the error and the first retry attempt (in
   * milliseconds).
   */
  int initial_backdown_period = 200;

  /**
   * @brief The backdown policy that should be used for the retry attempts.
   *
   * @deprecated Use \ref backdown_strategy instead. Will be removed by
   * 06.2020.
   *
   * @return The backdown policy.
   */
  BackdownPolicy backdown_policy = DefaultBackdownPolicy;

  /**
   * @brief The backdown strategy.
   *
   * Calculates the number of retry timeouts for failed requests. It is superior
   * to \ref backdown_policy as it computes the wait time based on the retry
   * count. By default, `backdown_strategy` is unset, and \ref backdown_policy
   * is used.
   *
   * @note You can use the \ref `ExponentialBackdownStrategy` as the new
   * backdown strategy.
   */
  BackdownStrategy backdown_strategy = nullptr;

  /**
   * @brief Evaluates responses to determine if the retry should be attempted.
   */
  RetryCondition retry_condition = DefaultRetryCondition;
};

/**
 * @brief Configures the behavior of the `OlpClient` class.
 */
struct CORE_API OlpClientSettings {
  /**
   * @brief The retry settings.
   */
  RetrySettings retry_settings;

  /**
   * @brief The network proxy settings.
   *
   * To remove any existing proxy settings, set to `boost::none`.
   */
  boost::optional<http::NetworkProxySettings> proxy_settings = boost::none;

  /**
   * @brief The authentication settings.
   *
   * To remove any existing authentication settings, set to `boost::none`.
   */
  boost::optional<AuthenticationSettings> authentication_settings = boost::none;

  /**
   * @brief The `TaskScheduler` instance.
   *
   * If `nullptr` is set, all request calls are performed synchronously.
   */
  std::shared_ptr<thread::TaskScheduler> task_scheduler = nullptr;

  /**
   * @brief The `Network` instance.
   *
   * Used to internally operate with the HERE platform Services.
   */
  std::shared_ptr<http::Network> network_request_handler = nullptr;

  /**
   * @brief The key-value cache that is used for storing different request
   * results such as metadata, partition data, URLs from the API Lookup Service,
   * and others.
   *
   * To only use the memory LRU cache with limited size, set to `nullptr`.
   */
  std::shared_ptr<cache::KeyValueCache> cache = nullptr;

  /**
   * @brief Set default expiration for any cache entry made by the according
   * layer or catalog client.
   *
   * This setting only applies to the mutable cache and to the in-memory cache,
   * but should not affect the protected cache as no entries are added to the
   * protected cache in read-only mode. Set to std::chrono::seconds::max() to
   * disable expiration. By default expiration is disabled.
   *
   * @note This only makes sense for data that has an expiration limit, e.g.
   * volatile or versioned, and which is stored in cache.
   */
  std::chrono::seconds default_cache_expiration = std::chrono::seconds::max();
};

}  // namespace client
}  // namespace olp
