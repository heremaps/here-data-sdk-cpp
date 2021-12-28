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

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/ErrorResponse.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/Types.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/OauthToken.h>
#include <olp/core/http/HttpStatusCode.h>

namespace olp {
namespace authentication {

static constexpr auto kDefaultMinimumValidity = 300ll;
static constexpr auto kDefaultMinimumValiditySeconds =
    std::chrono::seconds(kDefaultMinimumValidity);
static constexpr auto kForceRefresh = std::chrono::seconds(0);

namespace internal {

class TokenProviderPrivate;

/// An implementation of `TokenProvider`.
/// @note This is a private implementation class for internal use only, and not
/// bound to any API stability promises. Please do not use directly.
class AUTHENTICATION_API TokenProviderImpl {
 public:
  /**
   * @brief Creates the `TokenProviderImpl` instance.
   *
   * @param settings The `Settings` object that is used to customize
   * the `TokenEndpoint` instance.
   * @param minimum_validity Sets the minimum validity period of
   * the token in seconds.
   */
  TokenProviderImpl(Settings settings, std::chrono::seconds minimum_validity);

  /// @copydoc TokenProvider::operator()()
  std::string operator()() const;

  /// @copydoc TokenProvider::operator()(client::CancellationContext&)
  client::OauthTokenResponse operator()(
      client::CancellationContext& context) const;

  /// @copydoc TokenProvider::GetErrorResponse()
  ErrorResponse GetErrorResponse() const;

  /// @copydoc TokenProvider::GetHttpStatusCode()
  int GetHttpStatusCode() const;

  /// @copydoc TokenProvider::GetResponse()(client::CancellationContext&)
  TokenResponse GetResponse(client::CancellationContext& context) const;

  /// @copydoc TokenProvider::IsTokenResponseOK()
  bool IsTokenResponseOK() const;

 private:
  std::shared_ptr<TokenProviderPrivate> impl_;
};
}  // namespace internal

/**
 * @brief Provides the authentication tokens if the HERE platform
 * user credentials are valid.
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
      : impl_(std::make_shared<internal::TokenProviderImpl>(
            std::move(settings), std::chrono::seconds(MinimumValidity))) {}

  /// A default copy constructor.
  TokenProvider(const TokenProvider& other) = default;

  /// A default move constructor.
  TokenProvider(TokenProvider&& other) noexcept = default;

  /// A default copy assignment operator.
  TokenProvider& operator=(const TokenProvider& other) = default;

  /// A default move assignment operator.
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
   *
   * @deprecated Will be removed by 10.2022. Use the operator with
   * `CancellationContext` instead.
   */
  OLP_SDK_DEPRECATED(
      "Will be removed by 10.2022. Use the operator with `CancellationContext` "
      "instead.")
  std::string operator()() const { return impl_->operator()(); }

  /**
   * @brief Returns the access token or an error.
   *
   * @param context Used to cancel the pending token request.
   *
   * @returns An `OauthTokenResponse` if the response is successful; an
   * `ApiError` otherwise.
   */
  client::OauthTokenResponse operator()(
      client::CancellationContext& context) const {
    return impl_->operator()(context);
  }

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
  std::shared_ptr<internal::TokenProviderImpl> impl_;
};

/// Provides the authentication tokens using the default minimum token
/// validity.
using TokenProviderDefault = TokenProvider<kDefaultMinimumValidity>;

}  // namespace authentication
}  // namespace olp
