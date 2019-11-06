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

namespace {
constexpr auto kLogTag = "authentication::AutoRefreshingToken";

std::chrono::system_clock::time_point ComputeRefreshTime(
    time_t expiry_time, const std::chrono::seconds& minimum_validity) {
  auto expiry_time_chrono = std::chrono::system_clock::from_time_t(expiry_time);
  auto now = std::chrono::system_clock::now();
  return (expiry_time_chrono <= now) ? now
                                     : (expiry_time_chrono - minimum_validity);
}
}  // namespace

namespace olp {
namespace authentication {

struct AutoRefreshingToken::Impl {
  Impl(TokenEndpoint token_endpoint, TokenRequest token_request)
      : token_endpoint_(std::move(token_endpoint)),
        token_request_(std::move(token_request)),
        current_token_(),
        token_refresh_time_() {}

  TokenEndpoint::TokenResponse GetToken(
      client::CancellationToken& cancellation_token,
      std::chrono::seconds minimum_validity) {
    if (ForceRefresh(minimum_validity) || ShouldRefreshNow()) {
      OLP_SDK_LOG_INFO_F(kLogTag, "Time to refresh token");
      TryRefreshCurrentToken(cancellation_token, minimum_validity);
    }

    // Return existing still valid token
    return current_token_;
  }

  client::CancellationToken GetToken(const GetTokenCallback& callback,
                                     std::chrono::seconds minimum_validity) {
    if (ForceRefresh(minimum_validity) || ShouldRefreshNow()) {
      OLP_SDK_LOG_INFO_F(kLogTag, "Time to refresh token");
      return TryRefreshCurrentToken(callback, minimum_validity);
    }

    // Return existing still valid token
    callback(current_token_);
    return {};
  }

 private:
  bool ShouldRefreshNow() const {
    return std::chrono::system_clock::now() >= token_refresh_time_;
  }

  bool ForceRefresh(const std::chrono::seconds& minimum_validity) const {
    return minimum_validity <= std::chrono::seconds(0);
  }

  void TryRefreshCurrentToken(client::CancellationToken& cancellation_token,
                              const std::chrono::seconds& minimum_validity) {
    std::lock_guard<std::mutex> guard(token_mutex_);
    current_token_ =
        token_endpoint_.RequestToken(cancellation_token, token_request_).get();

    if (current_token_.IsSuccessful()) {
      auto expiry_time = current_token_.GetResult().GetExpiryTime();
      OLP_SDK_LOG_INFO_F(kLogTag, "Token OK, expires=%s",
                         std::asctime(std::gmtime(&expiry_time)));
    } else {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "Token NOK, code=%d, error=%s",
          static_cast<int>(current_token_.GetError().GetErrorCode()),
          current_token_.GetError().GetMessage().c_str());
    }

    token_refresh_time_ = ComputeRefreshTime(
        current_token_.GetResult().GetExpiryTime(), minimum_validity);
  }

  client::CancellationToken TryRefreshCurrentToken(
      const GetTokenCallback& callback,
      const std::chrono::seconds& minimum_validity) {
    std::lock_guard<std::mutex> guard(token_mutex_);

    // Avoid capturing this, but very risky as updating without lock
    auto& current_token = current_token_;
    auto& token_refresh_time = token_refresh_time_;

    return token_endpoint_.RequestToken(
        token_request_,
        [&, callback, minimum_validity](TokenEndpoint::TokenResponse response) {
          current_token = response;

          if (current_token.IsSuccessful()) {
            auto expiry_time = current_token.GetResult().GetExpiryTime();
            OLP_SDK_LOG_INFO_F(kLogTag, "Token OK, expires=%s",
                               std::asctime(std::gmtime(&expiry_time)));
          } else {
            OLP_SDK_LOG_INFO_F(
                kLogTag, "Token NOK, code=%d, error=%s",
                static_cast<int>(current_token.GetError().GetErrorCode()),
                current_token.GetError().GetMessage().c_str());
          }

          token_refresh_time = ComputeRefreshTime(
              current_token.GetResult().GetExpiryTime(), minimum_validity);
          callback(response);
        });
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
  return impl_->GetToken(cancellation_token, minimum_validity);
}

TokenEndpoint::TokenResponse AutoRefreshingToken::GetToken(
    const std::chrono::seconds& minimum_validity) const {
  client::CancellationToken cancellation_token;
  return impl_->GetToken(cancellation_token, minimum_validity);
}

client::CancellationToken AutoRefreshingToken::GetToken(
    const GetTokenCallback& callback,
    const std::chrono::seconds& minimum_validity) const {
  return impl_->GetToken(callback, minimum_validity);
}

}  // namespace authentication
}  // namespace olp
