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

#include <future>
#include <memory>

#include "AuthenticationApi.h"
#include "AuthenticationCredentials.h"
#include "AuthenticationError.h"
#include "Settings.h"
#include "TokenRequest.h"
#include "TokenResult.h"

#include "olp/core/client/ApiResponse.h"
#include "olp/core/client/CancellationToken.h"

namespace olp {
namespace authentication {
class AutoRefreshingToken;

/**
 * @brief The TokenEndpoint class directly corresponds to the token endpoint as
 * specified in the OAuth2.0 Specification.
 */
class AUTHENTICATION_API TokenEndpoint {
 public:
  /**
   * @brief RequestTokenCallback is a type that defines the signature used to
   * return the response to the client.
   */
  using TokenResponse = client::ApiResponse<TokenResult, AuthenticationError>;
  using RequestTokenCallback = std::function<void(TokenResponse)>;

  /**
   * POST to the token endpoint to get a HERE Access Token, for use with HERE
   * Services. Returns just the token, to be used as an Authorization: Bearer
   * token value.
   *
   * @param token_request the token request.
   * @param callback RequestTokenCallback for passing the TokenResponse back to
   * the caller.
   * @return CancellationToken that can be used to cancel the request.
   */
  client::CancellationToken RequestToken(
      const TokenRequest& token_request,
      const RequestTokenCallback& callback) const;

  /**
   * POST to the token endpoint to get a HERE Access Token, for use with HERE
   * Services. Returns just the token, to be used as an Authorization: Bearer
   * token value.
   *
   * @param cancellation_token that can be used to cancel the request.
   * @param token_request the token request.
   * @return A future object which will contain the TokenResponse.
   */
  std::future<TokenResponse> RequestToken(
      client::CancellationToken& cancellation_token,
      const TokenRequest& token_request = TokenRequest()) const;

  /**
   * POST to the token endpoint to get a HERE Access Token, for use with HERE
   * Services. Returns just the token, to be used as an Authorization: Bearer
   * token value.
   *
   * @param token_request the token request.
   * @return A future object which will contain the TokenResponse.
   */
  std::future<TokenResponse> RequestToken(
      const TokenRequest& token_request = TokenRequest()) const;

  /**
   * Obtain an AutoRefreshingToken which manages caching the requested token and
   * refreshing it when needed.
   *
   * @param token_request the token request.
   * @return An AutoRefreshingToken which manages token caching and refresh.
   */
  AutoRefreshingToken RequestAutoRefreshingToken(
      const TokenRequest& token_request = TokenRequest());

  /**
   * @brief Constructor
   * @param settings the settings object for this endpoint
   */
  explicit TokenEndpoint(Settings settings);

 private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace authentication
}  // namespace olp
