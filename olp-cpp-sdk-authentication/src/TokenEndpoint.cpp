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

#include "olp/authentication/TokenEndpoint.h"

PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

#include "olp/authentication/AuthenticationClient.h"
#include "olp/authentication/AuthenticationCredentials.h"
#include "olp/authentication/AutoRefreshingToken.h"
#include "olp/authentication/ErrorResponse.h"
#include "olp/authentication/SignInResult.h"
#include "olp/core/logging/Log.h"

namespace olp {
namespace authentication {

namespace {
static const std::string kOauth2TokenEndpoint = "/oauth2/token";
static constexpr auto kLogTag = "TokenEndpoint";

AuthenticationSettings ConvertSettings(Settings settings) {
  AuthenticationSettings auth_settings;
  auth_settings.network_proxy_settings = settings.network_proxy_settings;
  // Ignore task scheduler. It can cause a dealock on the sign in when used from
  // another task within `TaskScheduler` with 1 thread.
  //auth_settings.task_scheduler = settings.task_scheduler;
  auth_settings.network_request_handler = settings.network_request_handler;
  auth_settings.token_endpoint_url = settings.token_endpoint_url;
  auth_settings.use_system_time = settings.use_system_time;
  return auth_settings;
}
}  // namespace

class TokenEndpoint::Impl {
 public:
  explicit Impl(Settings settings);

  client::CancellationToken RequestToken(const TokenRequest& token_request,
                                         const RequestTokenCallback& callback);

  std::future<TokenResponse> RequestToken(
      client::CancellationToken& cancel_token,
      const TokenRequest& token_request);

 private:
  AuthenticationClient auth_client_;
  AuthenticationCredentials auth_credentials_;
};

TokenEndpoint::Impl::Impl(Settings settings)
    : auth_client_(ConvertSettings(settings)),
      auth_credentials_(std::move(settings.credentials)) {}

client::CancellationToken TokenEndpoint::Impl::RequestToken(
    const TokenRequest& token_request, const RequestTokenCallback& callback) {
  AuthenticationClient::SignInProperties properties;
  properties.expires_in = token_request.GetExpiresIn();
  return auth_client_.SignInClient(
      auth_credentials_, properties,
      [callback](
          const AuthenticationClient::SignInClientResponse& signInResponse) {
        if (signInResponse.IsSuccessful()) {
          TokenResult result(signInResponse.GetResult().GetAccessToken(),
                             signInResponse.GetResult().GetExpiresIn(),
                             signInResponse.GetResult().GetStatus(),
                             signInResponse.GetResult().GetErrorResponse());
          callback(TokenResponse(result));
        } else {
          callback(signInResponse.GetError());
        }
      });
}

std::future<TokenEndpoint::TokenResponse> TokenEndpoint::Impl::RequestToken(
    client::CancellationToken& cancel_token,
    const TokenRequest& token_request) {
  auto promise = std::make_shared<std::promise<TokenResponse>>();
  cancel_token = RequestToken(token_request, [promise](TokenResponse response) {
    promise->set_value(std::move(response));
  });
  return promise->get_future();
}

TokenEndpoint::TokenEndpoint(Settings settings) {
  // The underlying auth library expects a base URL and appends /oauth2/token
  // endpoint to it. Therefore if /oauth2/token is found it is stripped from the
  // endpoint URL provided. The underlying auth library should be updated to
  // support an arbitrary token endpoint URL.
  auto pos = settings.token_endpoint_url.find(kOauth2TokenEndpoint);
  if (pos != std::string::npos) {
    settings.token_endpoint_url.erase(pos, kOauth2TokenEndpoint.size());
  } else {
    OLP_SDK_LOG_ERROR(
        kLogTag,
        "Expected '/oauth2/token' endpoint in the token_endpoint_url. Only "
        "standard OAuth2 token endpoint URLs are supported.");
  }

  impl_ = std::make_shared<TokenEndpoint::Impl>(std::move(settings));
}

client::CancellationToken TokenEndpoint::RequestToken(
    const TokenRequest& token_request,
    const RequestTokenCallback& callback) const {
  return impl_->RequestToken(token_request, callback);
}

std::future<TokenEndpoint::TokenResponse> TokenEndpoint::RequestToken(
    client::CancellationToken& cancellation_token,
    const TokenRequest& token_request) const {
  return impl_->RequestToken(cancellation_token, token_request);
}

std::future<TokenEndpoint::TokenResponse> TokenEndpoint::RequestToken(
    const TokenRequest& token_request) const {
  client::CancellationToken cancellation_token;
  return impl_->RequestToken(cancellation_token, token_request);
}

AutoRefreshingToken TokenEndpoint::RequestAutoRefreshingToken(
    const TokenRequest& token_request) {
  return AutoRefreshingToken(*this, token_request);
}

PORTING_POP_WARNINGS()
}  // namespace authentication
}  // namespace olp
