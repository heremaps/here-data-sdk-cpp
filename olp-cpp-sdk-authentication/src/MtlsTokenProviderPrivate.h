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

#include <olp/authentication/MtlsTokenProvider.h>

#include <future>
#include <memory>
#include <mutex>
#include <thread>

#include <olp/authentication/AuthenticationClient.h>
#include <olp/authentication/AuthenticationSettings.h>
#include <olp/core/http/HttpStatusCode.h>
#include "AuthenticationClientImpl.h"

namespace olp {
namespace authentication {
namespace internal {

using SignInClientResponse = AuthenticationClient::SignInClientResponse;

class MtlsTokenProviderPrivate {
 public:
  MtlsTokenProviderPrivate(MtlsSettings settings,
                           std::chrono::seconds minimum_validity);

  client::OauthTokenResponse operator()(
      client::CancellationContext& context) const;

  ErrorResponse GetErrorResponse() const;

  int GetHttpStatusCode() const;

  bool IsTokenResponseOK() const;

 protected:
  static AuthenticationSettings MakeAuthenticationSettings(
      const MtlsSettings& settings);

  bool ShouldRefreshNow() const;

  SignInClientResponse GetResponse(client::CancellationContext& context) const;

  std::chrono::seconds minimum_validity_;
  MtlsProperties mtls_properties_;
  std::shared_ptr<AuthenticationClientImpl> client_;
  mutable SignInClientResponse current_token_;
  mutable std::chrono::steady_clock::time_point token_refresh_time_;
  mutable std::mutex request_mutex_;
};

}  // namespace internal
}  // namespace authentication
}  // namespace olp
