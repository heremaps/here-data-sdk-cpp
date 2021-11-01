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

#include <future>
#include <memory>

#include <olp/authentication/AuthenticationApi.h>
#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/AuthenticationError.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenRequest.h>
#include <olp/authentication/TokenResult.h>
#include <olp/authentication/Types.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/porting/warning_disable.h>

namespace olp {
namespace authentication {
class AutoRefreshingToken;
class TokenEndpointImpl;

/**
 * @brief Corresponds to the token endpoint as specified in the OAuth2.0
 * specification.
 */
class AUTHENTICATION_API OLP_SDK_DEPRECATED("Will be removed in 10.2020")
    TokenEndpoint {
 public:
  // Needed to avoid endless warnings from TokenRequest/TokenResult
  PORTING_PUSH_WARNINGS()
  PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

  /// Defines the signature used to return the response to the client.
  using TokenResponse = Response<TokenResult>;
  /// Defines the callback that is invoked when the response on
  /// `TokenRequest` is returned.
  using RequestTokenCallback = Callback<TokenResult>;

  /**
   * @brief Executes the POST request method to the token endpoint.
   *
   * The request gets the HERE Access token that is used to access the HERE
   * platform Services. Returns the token that is used as the `Authorization:
   * Bearer` token value.
   *
   * @param token_request The `TokenRequest` instance.
   * @param callback The `RequestTokenCallback` instance that passes
   * the `TokenResponse` instance back to the caller.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken RequestToken(
      const TokenRequest& token_request,
      const RequestTokenCallback& callback) const;

  /**
   * @brief Executes the POST request method to the token endpoint.
   *
   * The request gets the HERE Access token that is used to access the HERE platform
   * Services. Returns the token that is used as the `Authorization: Bearer`
   * token value.
   *
   * @param cancellation_token The `CancellationToken` instance that can be used
   * to cancel the request.
   * @param token_request The `TokenRequest` instance.
   *
   * @return The future object that contains the `TokenResponse` instance.
   */
  std::future<TokenResponse> RequestToken(
      client::CancellationToken& cancellation_token,
      const TokenRequest& token_request = TokenRequest()) const;

  /**
   * @brief Executes the POST request method to the token endpoint.
   *
   * The request gets the HERE Access token that is used to access the HERE
   * platform Services. Returns the token that is used as the `Authorization:
   * Bearer` token value.
   *
   * @param context Used to cancel the pending token request.
   * @param token_request The `TokenRequest` instance.
   *
   * @return The `TokenResponse` instance.
   */
  TokenResponse RequestToken(
      client::CancellationContext& context,
      const TokenRequest& token_request = TokenRequest()) const;

  /**
   * Executes the POST request method to the token endpoint.
   *
   * The request gets the HERE Access token that is used to access the HERE platform
   * Services. Returns the token that is used as the `Authorization: Bearer`
   * token value.
   *
   * @param token_request The `TokenRequest` instance.
   *
   * @return The future object that contains the `TokenResponse` instance.
   */
  std::future<TokenResponse> RequestToken(
      const TokenRequest& token_request = TokenRequest()) const;

  /**
   * @brief Gets the `AutoRefreshingToken` instance that caches the requested
   * token and refreshes it when needed.
   *
   * @param token_request The `TokenRequest` instance.
   *
   * @return The `AutoRefreshingToken` instance that caches the requested
   * token and refreshes it when needed.
   */
  AutoRefreshingToken RequestAutoRefreshingToken(
      const TokenRequest& token_request = TokenRequest());

  PORTING_POP_WARNINGS()

  /**
   * @brief Creates the `TokenEndpoint` instance with the given `settings`
   * parameter.
   *
   * @param settings The `Settings` object that is used to customize
   * the `TokenEndpoint` instance.
   */
  explicit TokenEndpoint(Settings settings);

 private:
  std::shared_ptr<TokenEndpointImpl> impl_;
};

}  // namespace authentication
}  // namespace olp
