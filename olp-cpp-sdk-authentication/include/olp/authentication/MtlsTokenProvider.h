/*
 * Copyright (C) 2026 HERE Europe B.V.
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
#include <utility>

#include <olp/authentication/ErrorResponse.h>
#include <olp/authentication/MtlsSettings.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/authentication/Types.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/OauthToken.h>
#include <olp/core/http/HttpStatusCode.h>

namespace olp {
namespace authentication {

namespace internal {

class MtlsTokenProviderPrivate;

/// An implementation of `MtlsTokenProvider`.
/// @note This is a private implementation class for internal use only, and
/// not bound to any API stability promises. Please do not use directly.
class AUTHENTICATION_API MtlsTokenProviderImpl {
 public:
  /**
   * @brief Creates the `MtlsTokenProviderImpl` instance.
   *
   * @param settings The `MtlsSettings` object that is used to customize the
   * mTLS token request.
   * @param minimum_validity Sets the minimum validity period of the token
   * in seconds.
   */
  MtlsTokenProviderImpl(MtlsSettings settings,
                        std::chrono::seconds minimum_validity);

  /// @copydoc MtlsTokenProvider::operator()(client::CancellationContext&)
  client::OauthTokenResponse operator()(
      client::CancellationContext& context) const;

  /// @copydoc MtlsTokenProvider::GetErrorResponse()
  ErrorResponse GetErrorResponse() const;

  /// @copydoc MtlsTokenProvider::GetHttpStatusCode()
  int GetHttpStatusCode() const;

  /// @copydoc MtlsTokenProvider::IsTokenResponseOK()
  bool IsTokenResponseOK() const;

 private:
  std::shared_ptr<MtlsTokenProviderPrivate> impl_;
};

}  // namespace internal

/**
 * @brief Provides authentication tokens using mTLS.
 *
 * @tparam MinimumValidity The minimum token validity time (in seconds).
 * To use the default `MinimumValidity` value, use the
 * `MtlsTokenProviderDefault` typedef.
 *
 * @see `MtlsTokenProviderDefault`
 */
template <uint64_t MinimumValidity>
class MtlsTokenProvider {
 public:
  /**
   * @brief Creates the `MtlsTokenProvider` instance with the `settings`
   * parameter.
   *
   * @param settings The settings that configure the mTLS token request.
   */
  explicit MtlsTokenProvider(MtlsSettings settings)
      : impl_(std::make_shared<internal::MtlsTokenProviderImpl>(
            std::move(settings), std::chrono::seconds(MinimumValidity))) {}

  /// A default copy constructor.
  MtlsTokenProvider(const MtlsTokenProvider& other) = default;

  /// A default move constructor.
  MtlsTokenProvider(MtlsTokenProvider&& other) noexcept = default;

  /// A default copy assignment operator.
  MtlsTokenProvider& operator=(const MtlsTokenProvider& other) = default;

  /// A default move assignment operator.
  MtlsTokenProvider& operator=(MtlsTokenProvider&& other) noexcept = default;

  /**
   * @brief Casts the `MtlsTokenProvider` instance to the `bool` type.
   *
   * Returns true if the previous token request was successful.
   *
   * @returns True if the previous token request was successful; false
   * otherwise.
   */
  operator bool() const { return impl_->IsTokenResponseOK(); }

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
  std::shared_ptr<internal::MtlsTokenProviderImpl> impl_;
};

/// Provides mTLS authentication tokens using the default minimum token
/// validity.
using MtlsTokenProviderDefault = MtlsTokenProvider<kDefaultMinimumValidity>;

}  // namespace authentication
}  // namespace olp
