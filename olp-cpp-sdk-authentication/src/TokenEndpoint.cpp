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

#include "TokenEndpoint.h"

#include <olp/authentication/AuthenticationClient.h>
#include <olp/core/logging/Log.h>
#include "AutoRefreshingToken.h"
#include "TokenEndpointImpl.h"
#include "TokenRequest.h"

namespace olp {
namespace authentication {

TokenEndpoint::TokenEndpoint(Settings settings)
    : impl_(std::make_shared<TokenEndpointImpl>(std::move(settings))) {}

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

}  // namespace authentication
}  // namespace olp
