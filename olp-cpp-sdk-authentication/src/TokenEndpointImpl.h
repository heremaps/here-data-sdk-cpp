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

#include <string>

#include <olp/authentication/AuthenticationClient.h>
#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/AuthenticationSettings.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/SignInResult.h>
#include <olp/authentication/Types.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/porting/optional.hpp>
#include "TokenRequest.h"

namespace olp {
namespace authentication {

/// @copydoc TokenEndpoint
class TokenEndpointImpl {
 public:
  /// Defines the callback that is invoked when the response on
  /// `TokenRequest` is returned.
  using RequestTokenCallback = Callback<TokenResult>;

  /// @copydoc TokenEndpoint::TokenEndpoint()
  explicit TokenEndpointImpl(Settings settings);

  /// @copydoc TokenEndpoint::RequestToken(const TokenRequest&, const
  /// RequestTokenCallback&)
  client::CancellationToken RequestToken(const TokenRequest& token_request,
                                         const RequestTokenCallback& callback);

  /// @copydoc TokenEndpoint::RequestToken(client::CancellationToken&, const
  /// TokenRequest&)
  std::future<TokenResponse> RequestToken(
      client::CancellationToken& cancel_token,
      const TokenRequest& token_request);

  /// @copydoc TokenEndpoint::RequestToken(client::CancellationContext&, const
  /// TokenRequest&)
  TokenResponse RequestToken(
      client::CancellationContext& context,
      const TokenRequest& token_request = TokenRequest()) const;

 private:
  class RequestTimer {
   public:
    RequestTimer();
    explicit RequestTimer(std::time_t server_time);
    std::time_t GetRequestTime() const;

   private:
    std::chrono::steady_clock::time_point timer_start_;
    std::time_t time_;
  };

  static SignInResult ParseAuthResponse(int status,
                                        std::stringstream& auth_response);

  Response<SignInResult> SignInClient(client::CancellationContext& context,
                                      const TokenRequest& token_request) const;

  client::HttpResponse CallAuth(const client::OlpClient& client,
                                const std::string& endpoint,
                                client::CancellationContext& context,
                                client::OlpClient::RequestBodyType body,
                                std::time_t timestamp) const;

  RequestTimer CreateRequestTimer(const client::OlpClient& client,
                                  client::CancellationContext& context) const;

  const AuthenticationCredentials credentials_;
  const porting::optional<std::string> scope_;
  const AuthenticationSettings settings_;
  AuthenticationClient auth_client_;
};

}  // namespace authentication
}  // namespace olp
