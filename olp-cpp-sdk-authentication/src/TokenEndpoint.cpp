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

#include <olp/authentication/TokenEndpoint.h>

PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

#include <olp/authentication/AuthenticationClient.h>
#include <olp/authentication/AutoRefreshingToken.h>
#include <olp/core/logging/Log.h>
#include "TokenEndpointImpl.h"

namespace olp {
namespace authentication {

namespace {
static const std::string kOauth2TokenEndpoint = "/oauth2/token";
static constexpr auto kLogTag = "TokenEndpoint";
}  // namespace

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

  impl_ = std::make_shared<TokenEndpointImpl>(std::move(settings));
}

client::CancellationToken TokenEndpoint::RequestToken(
    const TokenRequest& token_request,
    const RequestTokenCallback& callback) const {
  return impl_->RequestToken(token_request, callback);
}

std::future<TokenResponse> TokenEndpoint::RequestToken(
    client::CancellationToken& cancellation_token,
    const TokenRequest& token_request) const {
  return impl_->RequestToken(cancellation_token, token_request);
}

TokenResponse TokenEndpoint::RequestToken(
    client::CancellationContext& context,
    const TokenRequest& token_request) const {
  return impl_->RequestToken(context, token_request);
}

std::future<TokenResponse> TokenEndpoint::RequestToken(
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
