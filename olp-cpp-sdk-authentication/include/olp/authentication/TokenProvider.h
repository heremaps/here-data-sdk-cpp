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

#include <chrono>
#include <string>

#include "AuthenticationCredentials.h"
#include "AutoRefreshingToken.h"
#include "Settings.h"
#include "TokenEndpoint.h"
#include "TokenResult.h"

// Needed to avoid endless warnings from TokenEndpoint/TokenResponse
PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

namespace olp {
namespace authentication {
/** @file TokenProvider.h */

/**
 * @brief Provides the authentication tokens if the HERE Open Location Platform
 * (OLP) user credentials are valid.
 *
 * @param MinimumValidity The minimum token validity time (in seconds).
 * To use the default `MinimumValidity` value, use the `TokenProviderDefault`
 * typedef.
 *
 * @see `TokenProviderDefault`
 */
template <long long MinimumValidity>
class TokenProvider {
 public:
  /**
   * @brief Creates the `TokenProvider` instance with the `settings` parameter.
   *
   * @param settings The settings that can be used to configure
   * the `TokenEndpoint` instance.
   */
  explicit TokenProvider(Settings settings)
      : token_(
            TokenEndpoint(std::move(settings)).RequestAutoRefreshingToken()) {}

  /**
   * @brief Casts the `TokenProvider` instance to the `bool` type.
   *
   * Returns true if the previous token request was successful.
   *
   * @returns True if the previous token request was successful; false
   * otherwise.
   */
  operator bool() const { return IsTokenResponseOK(GetResponse()); }

  /**
   * @brief Casts the `TokenProvider` instance to the `std::string` type.
   *
   * Returns the access token string if the response is successful. Otherwise,
   * returns an empty string.
   *
   * @returns The access token string if the response is successful; an empty
   * string otherwise.
   */
  std::string operator()() const {
    return GetResponse().IsSuccessful()
               ? GetResponse().GetResult().GetAccessToken()
               : "";
  }

  /**
   * @brief Allows the `olp::client::ApiError` object associated
   * with the last request to be accessed if the token request is unsuccessful.
   *
   * @returns An error if the last token request failed.
   */
  ErrorResponse GetErrorResponse() const {
    return GetResponse().IsSuccessful()
               ? GetResponse().GetResult().GetErrorResponse()
               : ErrorResponse{};
  }

  /**
   * @brief Gets the HTTP status code of the last request.
   *
   * @returns The HTTP code of the last token request if it was successful.
   * Otherwise, returns the HTTP 503 Service Unavailable server error.
   */
  int GetHttpStatusCode() const {
    return GetResponse().IsSuccessful()
               ? GetResponse().GetResult().GetHttpStatus()
               : 503;  // ServiceUnavailable
  }

 private:
  TokenEndpoint::TokenResponse GetResponse() const {
    TokenEndpoint::TokenResponse resp =
        token_.GetToken(std::chrono::seconds{MinimumValidity});
    return resp;
  }

  bool IsTokenResponseOK(const TokenEndpoint::TokenResponse& resp) const {
    return resp.IsSuccessful() &&
           resp.GetResult().GetErrorResponse().code == 0ul;
  }

  AutoRefreshingToken token_;
};

/**
 * @brief Provides the authentication tokens using the default minimum token
 * validity.
 *
 * The HERE OLP user credentials should be valid.
 */
using TokenProviderDefault = TokenProvider<kDefaultMinimumValidity>;

PORTING_POP_WARNINGS()
}  // namespace authentication
}  // namespace olp
