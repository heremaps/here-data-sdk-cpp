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

#include "olp/authentication/AutoRefreshingToken.h"

#include <chrono>
#include <iostream>

#include "olp/authentication/TokenEndpoint.h"
#include "olp/core/client/CancellationToken.h"
#include "olp/core/logging/Log.h"

#define LOG_TAG "here::account::oauth2::AutoRefreshingToken"
#define REQUEST_WAIT_TIME_INTERVAL_MS 100

namespace olp {
namespace authentication {

struct AutoRefreshingToken::Impl {
  Impl(TokenEndpoint tokenEndpoint, TokenRequest tokenRequest)
      : token_endpoint_(std::move(tokenEndpoint)),
        token_request_(std::move(tokenRequest)),
        current_token_(),
        token_refresh_time_() {}

  TokenEndpoint::TokenResponse getToken(
      client::CancellationToken& cancellationToken,
      const std::chrono::seconds& minimumValidity) {
    if (forceRefresh(minimumValidity) || shouldRefreshNow()) {
      tryRefreshCurrentToken(cancellationToken, minimumValidity);
    }

    return current_token_;
  }

  client::CancellationToken getToken(
      const GetTokenCallback& callback,
      const std::chrono::seconds& minimumValidity) {
    if (forceRefresh(minimumValidity) || shouldRefreshNow()) {
      return tryRefreshCurrentToken(callback, minimumValidity);
    } else {
      callback(current_token_);
    }
    return client::CancellationToken();
  }

 private:
  bool shouldRefreshNow() const {
    return std::chrono::system_clock::now() >= token_refresh_time_;
  }

  bool forceRefresh(const std::chrono::seconds& minimumValidity) const {
    return minimumValidity <= std::chrono::seconds(0);
  }

  void tryRefreshCurrentToken(client::CancellationToken& cancellationToken,
                              const std::chrono::seconds& minimumValidity) {
    std::lock_guard<std::mutex> guard(token_mutex_);
    if (!forceRefresh(minimumValidity) && !shouldRefreshNow()) {
      return;
    }
    current_token_ =
        token_endpoint_.RequestToken(cancellationToken, token_request_).get();

    token_refresh_time_ = computeTokenRefreshTime(
        current_token_.GetResult().GetExpiryTime(), minimumValidity);
  }

  client::CancellationToken tryRefreshCurrentToken(
      const GetTokenCallback& callback,
      const std::chrono::seconds& minimumValidity) {
    std::lock_guard<std::mutex> guard(token_mutex_);
    if (!forceRefresh(minimumValidity) && !shouldRefreshNow()) {
      callback(current_token_);
      return client::CancellationToken();
    }
    auto& currentToken = current_token_;
    auto& tokenRefreshTime = token_refresh_time_;
    return token_endpoint_.RequestToken(
        token_request_,
        [callback, &currentToken, &tokenRefreshTime,
         minimumValidity](TokenEndpoint::TokenResponse tokenResponse) {
          currentToken = tokenResponse;
          tokenRefreshTime = computeTokenRefreshTime(
              currentToken.GetResult().GetExpiryTime(), minimumValidity);
          callback(tokenResponse);
        });
  }

  static std::chrono::system_clock::time_point computeTokenRefreshTime(
      const time_t& expiryTime, const std::chrono::seconds& minimumValidity) {
    auto expiryTimeChrono = std::chrono::system_clock::from_time_t(expiryTime);
    if (expiryTimeChrono <= std::chrono::system_clock::now()) {
      return std::chrono::system_clock::now();
    }

    return expiryTimeChrono - minimumValidity;
  }

 private:
  TokenEndpoint token_endpoint_;
  TokenRequest token_request_;
  TokenEndpoint::TokenResponse current_token_;

  std::chrono::system_clock::time_point token_refresh_time_;
  std::mutex token_mutex_;
};

AutoRefreshingToken::AutoRefreshingToken(TokenEndpoint token_endpoint,
                                         TokenRequest token_request)
    : impl_(std::make_shared<AutoRefreshingToken::Impl>(
          std::move(token_endpoint), std::move(token_request))) {}

TokenEndpoint::TokenResponse AutoRefreshingToken::GetToken(
    client::CancellationToken& cancellation_token,
    const std::chrono::seconds& minimum_validity) const {
  return impl_->getToken(cancellation_token, minimum_validity);
}

TokenEndpoint::TokenResponse AutoRefreshingToken::GetToken(
    const std::chrono::seconds& minimum_validity) const {
  client::CancellationToken cancellationToken;
  return impl_->getToken(cancellationToken, minimum_validity);
}

client::CancellationToken AutoRefreshingToken::GetToken(
    const GetTokenCallback& callback,
    const std::chrono::seconds& minimum_validity) const {
  return impl_->getToken(callback, minimum_validity);
}

}  // namespace authentication
}  // namespace olp
