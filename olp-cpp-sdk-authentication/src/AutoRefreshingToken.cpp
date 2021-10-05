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

#include "olp/authentication/AutoRefreshingToken.h"

#include <chrono>
#include <iostream>

#include "olp/authentication/TokenEndpoint.h"
#include "olp/core/client/CancellationToken.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/warning_disable.h"

namespace {
constexpr auto kLogTag = "authentication::AutoRefreshingToken";

std::chrono::steady_clock::time_point ComputeRefreshTime(
    const olp::authentication::TokenResponse& current_token,
    const std::chrono::seconds& minimum_validity) {
  auto now = std::chrono::steady_clock::now();

  if (!current_token) {
    return now;
  }

  auto expiry_time_chrono = now + current_token.GetResult().GetExpiresIn();
  return (expiry_time_chrono <= now) ? now
                                     : (expiry_time_chrono - minimum_validity);
}
}  // namespace

namespace olp {
namespace authentication {
PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

struct AutoRefreshingToken::Impl {
  Impl(TokenEndpoint token_endpoint, TokenRequest token_request)
      : token_endpoint_(std::move(token_endpoint)),
        token_request_(std::move(token_request)),
        current_token_(),
        token_refresh_time_() {}

  TokenResponse GetToken(client::CancellationToken& cancellation_token,
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
    return std::chrono::steady_clock::now() >= token_refresh_time_;
  }

  bool ForceRefresh(const std::chrono::seconds& minimum_validity) const {
    return minimum_validity <= std::chrono::seconds(0);
  }

  void TryRefreshCurrentToken(client::CancellationToken& cancellation_token,
                              const std::chrono::seconds& minimum_validity) {
    std::lock_guard<std::mutex> guard(token_mutex_);
    current_token_ =
        token_endpoint_.RequestToken(cancellation_token, token_request_).get();

    if (!current_token_.IsSuccessful()) {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "Token NOK, error_code=%d, http_status=%d, message='%s'",
          static_cast<int>(current_token_.GetError().GetErrorCode()),
          current_token_.GetError().GetHttpStatusCode(),
          current_token_.GetError().GetMessage().c_str());
    } else {
      auto expiry_time = current_token_.GetResult().GetExpiryTime();
      OLP_SDK_LOG_INFO_F(kLogTag, "Token OK, expires=%s",
                         std::asctime(std::gmtime(&expiry_time)));
    }

    token_refresh_time_ = ComputeRefreshTime(current_token_, minimum_validity);
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
        [&, callback, minimum_validity](TokenResponse response) {
          current_token = std::move(response);

          if (!current_token.IsSuccessful()) {
            OLP_SDK_LOG_INFO_F(
                kLogTag,
                "Token NOK, error_code=%d, http_status=%d, message='%s'",
                static_cast<int>(current_token.GetError().GetErrorCode()),
                current_token.GetError().GetHttpStatusCode(),
                current_token.GetError().GetMessage().c_str());
          } else {
            auto expiry_time = current_token.GetResult().GetExpiryTime();
            OLP_SDK_LOG_INFO_F(kLogTag, "Token OK, expires=%s",
                               std::asctime(std::gmtime(&expiry_time)));
          }

          token_refresh_time =
              ComputeRefreshTime(current_token, minimum_validity);
          callback(current_token);
        });
  }

 private:
  TokenEndpoint token_endpoint_;
  TokenRequest token_request_;
  TokenResponse current_token_;
  std::chrono::steady_clock::time_point token_refresh_time_;
  std::mutex token_mutex_;
};

AutoRefreshingToken::AutoRefreshingToken(TokenEndpoint token_endpoint,
                                         TokenRequest token_request)
    : impl_(std::make_shared<AutoRefreshingToken::Impl>(
          std::move(token_endpoint), std::move(token_request))) {}

TokenResponse AutoRefreshingToken::GetToken(
    client::CancellationToken& cancellation_token,
    const std::chrono::seconds& minimum_validity) const {
  return impl_->GetToken(cancellation_token, minimum_validity);
}

TokenResponse AutoRefreshingToken::GetToken(
    const std::chrono::seconds& minimum_validity) const {
  client::CancellationToken cancellation_token;
  return impl_->GetToken(cancellation_token, minimum_validity);
}

client::CancellationToken AutoRefreshingToken::GetToken(
    const GetTokenCallback& callback,
    const std::chrono::seconds& minimum_validity) const {
  return impl_->GetToken(callback, minimum_validity);
}
PORTING_POP_WARNINGS()

}  // namespace authentication
}  // namespace olp
