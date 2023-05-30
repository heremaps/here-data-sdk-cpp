/*
 * Copyright (C) 2019-2023 HERE Europe B.V.
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

#include <olp/authentication/TokenProvider.h>

#include <olp/authentication/ErrorResponse.h>
#include "AutoRefreshingToken.h"
#include "TokenEndpoint.h"

namespace olp {
namespace authentication {
namespace internal {

class TokenProviderPrivate {
 public:
  TokenProviderPrivate(Settings settings, std::chrono::seconds minimum_validity)
      : minimum_validity_{minimum_validity},
        token_(std::make_shared<AutoRefreshingToken>(
            TokenEndpoint(std::move(settings)), TokenRequest())) {}

  std::string operator()() const {
    client::CancellationContext context;
    const auto response = GetResponse(context);
    return response ? response.GetResult().GetAccessToken() : "";
  }

  client::OauthTokenResponse operator()(
      client::CancellationContext& context) const {
    const auto response = GetResponse(context);
    return response ? client::OauthTokenResponse(
                          {response.GetResult().GetAccessToken(),
                           response.GetResult().GetExpiryTime()})
                    : client::OauthTokenResponse(response.GetError());
  }

  ErrorResponse GetErrorResponse() const {
    client::CancellationContext context;
    const auto response = GetResponse(context);
    if (response) {
      return ErrorResponse{};
    }

    ErrorResponse error_response;
    error_response.message = response.GetError().GetMessage();

    return error_response;
  }

  int GetHttpStatusCode() const {
    client::CancellationContext context;
    const auto response = GetResponse(context);
    return response ? http::HttpStatusCode::OK
                    : response.GetError().GetHttpStatusCode();
  }

  TokenResponse GetResponse(client::CancellationContext& context) const {
    // Prevents multiple authorization requests that can
    // happen when the token is not available, and multiple consumers
    // request it.
    std::lock_guard<std::mutex> lock(request_mutex_);
    return token_->GetToken(context, minimum_validity_);
  }

  /// Checks whether the available token response is valid.
  bool IsTokenResponseOK() const {
    client::CancellationContext context;

    // The token response is successful only if the token is valid.
    return GetResponse(context).IsSuccessful();
  }

 private:
  std::chrono::seconds minimum_validity_;
  std::shared_ptr<AutoRefreshingToken> token_;
  mutable std::mutex request_mutex_;
};

TokenProviderImpl::TokenProviderImpl(Settings settings,
                                     std::chrono::seconds minimum_validity)
    : impl_(std::make_shared<TokenProviderPrivate>(
          std::move(settings), std::chrono::seconds(minimum_validity))) {}

client::OauthTokenResponse TokenProviderImpl::operator()(
    client::CancellationContext& context) const {
  return impl_->operator()(context);
}

ErrorResponse TokenProviderImpl::GetErrorResponse() const {
  return impl_->GetErrorResponse();
}

int TokenProviderImpl::GetHttpStatusCode() const {
  return impl_->GetHttpStatusCode();
}

TokenResponse TokenProviderImpl::GetResponse(
    client::CancellationContext& context) const {
  return impl_->GetResponse(context);
}

bool TokenProviderImpl::IsTokenResponseOK() const {
  return impl_->IsTokenResponseOK();
}

}  // namespace internal
}  // namespace authentication
}  // namespace olp
