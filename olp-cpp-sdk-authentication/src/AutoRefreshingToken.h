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

#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include <olp/authentication/AuthenticationApi.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include "TokenEndpoint.h"

namespace olp {
namespace authentication {

/**
 * @brief Manages token requests.
 *
 * Requests a new token from the token endpoint and automatically refreshes it
 * when the token is about to expire.
 */
class AutoRefreshingToken {
 public:
  /**
   * @brief Specifies the callback signature that is required
   * when the get token request is completed.
   */
  using GetTokenCallback = std::function<void(const TokenResponse& response)>;

  /**
   * @brief Synchronously gets a token that is always fresh.
   *
   * If no token has been retrieved yet or the current token is expired or
   * expires within five minutes, a new token is requested. Otherwise,
   * the cached token is returned. This method is thread-safe.
   *
   * @note This method is blocked when a new token needs to be retrieved.
   * Therefore, the token should not be called from a time-sensitive thread (for
   * example, the UI thread).
   *
   * @param cancellation_token The token used to cancel the operation.
   * @param minimum_validity (Optional) Sets the minimum validity period of
   * the token in seconds. The default validity period is 5 minutes. If
   * the period is set to 0, the token is refreshed immediately.
   *
   * @return The `TokenResponse` instance if the old one is expired.
   * Otherwise, returns the cached `TokenResponse` instance.
   */
  TokenResponse GetToken(client::CancellationToken& cancellation_token,
                         const std::chrono::seconds& minimum_validity) const;

  /**
   * @brief Synchronously gets a token that is always fresh.
   *
   * If no token has been retrieved yet or the current token is expired or
   * expires within five minutes, a new token is requested. Otherwise,
   * the cached token is returned. This method is thread-safe.
   * @note This method is blocked when a new token needs to be retrieved.
   * Therefore, the token should not be called from a time-sensitive thread (for
   * example, the UI thread).
   *
   * @param context Used to cancel the pending token request.
   * @param minimum_validity (Optional) Sets the minimum validity period of
   * the token in seconds. The default validity period is 5 minutes. If
   * the period is set to 0, the token is refreshed immediately.
   *
   * @return The `TokenResponse` instance if the old one is expired.
   * Otherwise, returns the cached `TokenResponse` instance.
   */
  TokenResponse GetToken(client::CancellationContext& context,
                         const std::chrono::seconds& minimum_validity) const;

  /**
   * @brief Synchronously gets a token that is always fresh.
   *
   * If no token has been retrieved yet or the current token is expired or
   * expires within five minutes, a new token is requested. Otherwise,
   * the cached token is returned. This method is thread-safe.
   * @note This method is blocked when a new token needs to be retrieved.
   * Therefore, it should not be called from a time-sensitive thread (for
   * example, the UI thread).
   *
   * @param minimum_validity (Optional) Sets the minimum validity period of
   * the token in seconds. The default validity period is 5 minutes. If
   * the period is set to 0, the token is refreshed immediately.
   *
   * @return The `TokenResponse` instance if the old one is expired.
   * Otherwise, the cached `TokenResponse` instance.
   */
  TokenResponse GetToken(const std::chrono::seconds& minimum_validity) const;

  /**
   * @brief Asynchronously gets a token that is always fresh.
   *
   * If no token has been retrieved yet or the current token is expired or
   * expires within five minutes, a new token is requested. Otherwise,
   * the cached token is returned. This method is thread-safe.
   *
   * @param callback The callback that contains the `TokenResponse` instance.
   * @param minimum_validity (Optional) Sets the minimum validity period of
   * the token in seconds. The default validity period is 5 minutes. If
   * the period is set to 0, the token is refreshed immediately.
   *
   * @return The `CancellationToken` instance that is used to cancel
   * the operation.
   */
  client::CancellationToken GetToken(
      const GetTokenCallback& callback,
      const std::chrono::seconds& minimum_validity) const;

  /**
   * @brief Creates the `AutoRefreshingToken` instance.
   *
   * @param token_endpoint The token endpoint against which the token is
   * refreshed.
   * @param token_request The token request that is sent to the token endpoint.
   */
  AutoRefreshingToken(TokenEndpoint token_endpoint, TokenRequest token_request);

 private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace authentication
}  // namespace olp
