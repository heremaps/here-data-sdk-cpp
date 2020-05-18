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

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/AutoRefreshingToken.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenEndpoint.h>
#include <olp/authentication/TokenResult.h>
#include <olp/core/http/HttpStatusCode.h>

namespace olp {
namespace authentication {

// Needed to avoid endless warnings from TokenRequest/TokenResult
PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

/**
 * @brief Provides the authentication tokens if the HERE Open Location Platform
 * (OLP) user credentials are valid.
 *
 * @tparam MinimumValidity The minimum token validity time (in seconds).
 * To use the default `MinimumValidity` value, use the `TokenProviderDefault`
 * typedef.
 *
 * @see `TokenProviderDefault`
 */
template <uint64_t MinimumValidity>
class TokenProvider {
 public:
  /**
   * @brief Creates the `TokenProvider` instance with the `settings` parameter.
   *
   * @param settings The settings that can be used to configure
   * the `TokenEndpoint` instance.
   */
  explicit TokenProvider(Settings settings)
      : impl_(std::make_shared<TokenProviderImpl>(
            std::move(settings), std::chrono::seconds(MinimumValidity))) {}

  // Default copyable, default movable.
  TokenProvider(const TokenProvider& other) = default;
  TokenProvider(TokenProvider&& other) noexcept = default;
  TokenProvider& operator=(const TokenProvider& other) = default;
  TokenProvider& operator=(TokenProvider&& other) noexcept = default;

  /**
   * @brief Casts the `TokenProvider` instance to the `bool` type.
   *
   * Returns true if the previous token request was successful.
   *
   * @returns True if the previous token request was successful; false
   * otherwise.
   */
  operator bool() const { return impl_->IsTokenResponseOK(); }

  /**
   * @brief Casts the `TokenProvider` instance to the `std::string` type.
   *
   * Returns the access token string if the response is successful. Otherwise,
   * returns an empty string.
   *
   * @returns The access token string if the response is successful; an empty
   * string otherwise.
   */
  std::string operator()() const { return impl_->operator()(); }

  /**
   * @brief Allows the `olp::client::ApiError` object associated
   * with the last request to be accessed if the token request is unsuccessful.
   *
   * @returns An error if the last token request failed.
   */
  ErrorResponse GetErrorResponse() const { return impl_->GetErrorResponse(); }

  /**
   * @brief Gets the HTTP status code of the last request.
   *
   * @returns The HTTP code of the last token request if it was successful.
   * Otherwise, returns the HTTP 503 Service Unavailable server error.
   */
  int GetHttpStatusCode() const { return impl_->GetHttpStatusCode(); }

 private:
  class TokenProviderImpl {
   public:
    static constexpr auto kValidTokenResponseCode = 0ul;
    using TokenResponse = TokenEndpoint::TokenResponse;

    explicit TokenProviderImpl(Settings settings,
                               std::chrono::seconds minimum_validity)
        : minimum_validity_{minimum_validity},
          token_(
              TokenEndpoint(std::move(settings)).RequestAutoRefreshingToken()) {
    }

    /// @copydoc TokenProvider::operator()()
    std::string operator()() const {
      return GetResponse().IsSuccessful()
                 ? GetResponse().GetResult().GetAccessToken()
                 : "";
    }

    /// @copydoc TokenProvider::GetErrorResponse()
    ErrorResponse GetErrorResponse() const {
      auto response = GetResponse();
      return response.IsSuccessful() ? response.GetResult().GetErrorResponse()
                                     : ErrorResponse{};
    }

    /// @copydoc TokenProvider::GetHttpStatusCode()
    int GetHttpStatusCode() const {
      auto response = GetResponse();
      return response.IsSuccessful()
                 ? response.GetResult().GetHttpStatus()
                 : http::HttpStatusCode::SERVICE_UNAVAILABLE;
    }

    /// Get the token response from AutoRefreshingToken or request a new token
    /// if expired or not present.
    TokenResponse GetResponse() const {
      /// Mutex is needed to prevent multiple authorization requests, that could
      /// happen when the token is not yet available, and multiple consumers
      /// requested it.
      std::lock_guard<std::mutex> lock(request_mutex_);
      return token_.GetToken(minimum_validity_);
    }

    /// Check if the available token response is valid, i.e. error code is 0.
    bool IsTokenResponseOK() const {
      const auto response = GetResponse();
      return response.IsSuccessful() &&
             response.GetResult().GetErrorResponse().code ==
                 kValidTokenResponseCode;
    }

   private:
    std::chrono::seconds minimum_validity_{kDefaultMinimumValidity};
    AutoRefreshingToken token_;
    mutable std::mutex request_mutex_;
  };

  std::shared_ptr<TokenProviderImpl> impl_;
};

/// Provides the authentication tokens using the default minimum token
/// validity.
using TokenProviderDefault = TokenProvider<kDefaultMinimumValidity>;

PORTING_POP_WARNINGS()
}  // namespace authentication
}  // namespace olp
