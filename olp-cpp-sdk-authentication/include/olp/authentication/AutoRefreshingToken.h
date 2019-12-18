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

#include <chrono>
#include <functional>
#include <memory>

#include "AuthenticationApi.h"
#include "TokenEndpoint.h"
#include "TokenRequest.h"
#include "TokenResult.h"

#include "olp/core/client/CancellationToken.h"
#include "olp/core/porting/warning_disable.h"

namespace olp {
namespace authentication {

static constexpr auto kDefaultMinimumValidity = 300ll;
static constexpr auto kDefaultMinimumValiditySeconds =
    std::chrono::seconds(kDefaultMinimumValidity);
static constexpr auto kForceRefresh = std::chrono::seconds(0);

/**
 * @brief The AutoRefreshingToken class manages caching the requested token and
 * refreshing it when needed.
 */
class AUTHENTICATION_API AutoRefreshingToken {
 public:
  /**
   * @brief The getTokenCallback type specifies the callback signature required
   * for when the get token request is completed.
   */
  using GetTokenCallback =
      std::function<void(const TokenEndpoint::TokenResponse& response)>;

  /**
   * Synchronously gets a token which is always fresh. If no token has been
   * retrieved yet or the current token is expired or will expire within five
   * minutes a new token is requested, otherwise the cached token is returned.
   * This method is thread safe.
   * @note This method blocks when a new token needs to be retrieved. Therefore
   * it should not be called from a time sensitive thread (e.g. UI thread).
   * @throw This method might throw a std::future_error exception.
   *
   * @param cancellation_token a token used to cancel the operation.
   * @param minimum_validity Optional. Sets the minimum validity period of the
   * token in seconds, The default validity period is 5 minutes. If the period
   * is set to 0, the token will be refreshed immediately.
   * @return A new TokenResponse if the old one is expired. Otherwise the cached
   * token response.
   */

  TokenEndpoint::TokenResponse GetToken(
      client::CancellationToken& cancellation_token,
      const std::chrono::seconds& minimum_validity =
          kDefaultMinimumValiditySeconds) const;

  /**
   * Synchronously gets a token which is always fresh. If no token has been
   * retrieved yet or the current token is expired or will expire within five
   * minutes a new token is requested, otherwise the cached token is returned.
   * This method is thread safe.
   * @note This method blocks when a new token needs to be retrieved. Therefore
   * it should not be called from a time sensitive thread (e.g. UI thread).
   * @throw This method might throw a std::future_error exception.
   *
   * @param minimum_validity Optional. Sets the minimum validity period of the
   * token in seconds, The default validity period is 5 minutes. If the period
   * is set to 0, the token will be refreshed immediately.
   * @return A new TokenResponse if the old one is expired. Otherwise the cached
   * token response.
   */
  TokenEndpoint::TokenResponse GetToken(
      const std::chrono::seconds& minimum_validity =
          kDefaultMinimumValiditySeconds) const;

  /**
   * Aysnchronously gets a token which is always fresh. If no token has been
   * retrieved yet or the current token is expired or will expire within five
   * minutes a new token is requested, otherwise the cached token is returned.
   * This method is thread safe.
   *
   * @param callback A callback containing the TokenResponse.
   * @param minimum_validity Optional. Sets the minimum validity period of the
   * token in seconds, The default validity period is 5 minutes. If the period
   * is set to 0, the token will be refreshed immediately.
   * @return CancellationToken a token used to cancel the operation.
   */
  client::CancellationToken GetToken(
      const GetTokenCallback& callback,
      const std::chrono::seconds& minimum_validity =
          kDefaultMinimumValiditySeconds) const;

  /**
   * @brief Constructor
   * @param token_endpoint the token endpoint to refresh against
   * @param token_request the token request to send to the token endpoint
   */
  PORTING_PUSH_WARNINGS()
  PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")
  AutoRefreshingToken(TokenEndpoint token_endpoint, TokenRequest token_request);
  PORTING_POP_WARNINGS()

 private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace authentication
}  // namespace olp
