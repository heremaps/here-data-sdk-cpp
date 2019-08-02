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

namespace olp {
namespace authentication {
/** @file TokenProvider.h */

/**
 * Use TokenProviderDefault typedef to use default minimumValidity value. @see
 * TokenProviderDefault
 * @brief Provides authentication tokens given appropriate OLP user credentials.
 * @param minimumValidity the minimum token validity to use in seconds.
 */
template <long long MinimumValidity>
class TokenProvider {
 public:
  /**
   * @brief Constructor
   * @param key The client access key identifier.
   * @param secret The client access key secret.
   */
  TokenProvider(const std::string& key, const std::string& secret)
      : TokenProvider(key, secret, Settings{}) {}

  /**
   * @brief Constructor
   * @param key The client access key identifier.
   * @param secret The client access key secret.
   * @param settings Settings which can be used to configure a TokenEndpoint
   * instance.
   */
  TokenProvider(const std::string& key, const std::string& secret,
                Settings settings)
      : key_(key),
        secret_(secret),
        settings_(settings),
        token_(TokenEndpoint({key_, secret_}, settings_)
                   .RequestAutoRefreshingToken()) {}

  /**
   * @brief bool type conversion
   * @returns true if previous token request was successful.
   */
  operator bool() const { return IsTokenResponseOK(GetResponse()); }

  /**
   * @brief Retrieves an access token and returns as a string to the user.
   * @returns the access token.
   */
  std::string operator()() const {
    return GetResponse().IsSuccessful()
               ? GetResponse().GetResult().GetAccessToken()
               : "";
  }

  /**
   * @brief Allows the olp::client::ApiError object associated
   * with the last request to be accessed if token request is unsuccessful.
   * @returns the error if the last token request failed.
   */
  ErrorResponse GetErrorResponse() const {
    return GetResponse().IsSuccessful()
               ? GetResponse().GetResult().GetErrorResponse()
               : ErrorResponse{};
  }

  /**
   * @brief Allows the http status code of the last request to be retrieved.
   * @returns http status code
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

  std::string key_;
  std::string secret_;
  Settings settings_;
  AutoRefreshingToken token_;
};

/**
 * @brief TokenProviderDefault - provides authentication tokens given
 * appropriate OLP user credentials using the default minimum token validity.
 */
using TokenProviderDefault = TokenProvider<kDefaultMinimumValidity>;

}  // namespace authentication
}  // namespace olp
