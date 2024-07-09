/*
 * Copyright (C) 2021-2024 HERE Europe B.V.
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

#include <olp/core/client/BackdownStrategy.h>
#include <olp/core/client/HttpResponse.h>

namespace olp {
namespace client {

/**
 * @brief The default retry condition that disables retries.
 */
CORE_API bool DefaultRetryCondition(const olp::client::HttpResponse& response);

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
   * @brief Maximum time for request to complete (in seconds).
   *
   * The default value is 60.
   *
   * @note Connection or data transfer will be interrupted after specified
   * period of time ignoring `connection_timeout` and `transfer_timeout` values.
   */
  int timeout = 60;

#ifdef OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD
  /**
   * @brief Maximum time for background request to complete (in seconds).
   *
   * The default value is 600.
   *
   * @note Connection or data transfer will be interrupted after specified
   * period of time ignoring `connection_timeout` and `transfer_timeout` values.
   */
  int background_timeout = 600;
#endif  // OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD

  /**
   * @brief Time allowed to wait for connection to establish (in milliseconds).
   *
   * The default value is 30 seconds.
   *
   * @note The value should be smaller than `timeout`, @see `timeout` for more
   * details.
   */
  std::chrono::milliseconds connection_timeout = std::chrono::seconds(30);

#ifdef OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD
  /**
   * @brief Time allowed to wait for background connection to establish (in
   * milliseconds).
   *
   * The default value is 600 seconds.
   *
   * @note The value should be smaller than `timeout`, @see `timeout` for more
   * details.
   */
  std::chrono::milliseconds background_connection_timeout =
      std::chrono::seconds(600);
#endif  // OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD

  /**
   * @brief Time allowed to wait without data transfer (in milliseconds).
   *
   * The default value is 30 seconds.
   *
   * @note The value should be smaller than `timeout`, @see `timeout` for more
   * details.
   */
  std::chrono::milliseconds transfer_timeout = std::chrono::seconds(30);

  /**
   * @brief The period between the error and the first retry attempt (in
   * milliseconds).
   */
  int initial_backdown_period = 200;

  /**
   * @brief The backdown strategy.
   *
   * Defines the delay between retries on a failed request.
   */
  BackdownStrategy backdown_strategy = ExponentialBackdownStrategy();

  /**
   * @brief Evaluates responses to determine if the retry should be attempted.
   */
  RetryCondition retry_condition = DefaultRetryCondition;
};

}  // namespace client
}  // namespace olp
