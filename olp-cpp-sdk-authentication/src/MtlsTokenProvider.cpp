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

#include <olp/authentication/MtlsTokenProvider.h>

#include <future>
#include <mutex>
#include <thread>

#include <olp/authentication/AuthenticationClient.h>
#include <olp/authentication/AuthenticationSettings.h>
#include <olp/core/http/HttpStatusCode.h>

#include "MtlsTokenProviderPrivate.h"

namespace olp {
namespace authentication {
namespace internal {

namespace {

std::chrono::steady_clock::time_point ComputeRefreshTime(
    const SignInClientResponse& current_token,
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

MtlsTokenProviderPrivate::MtlsTokenProviderPrivate(
    MtlsSettings settings, std::chrono::seconds minimum_validity)
    : minimum_validity_(minimum_validity),
      mtls_properties_(settings.mtls_properties),
      client_(std::make_shared<AuthenticationClientImpl>(
          MakeAuthenticationSettings(settings))) {}

client::OauthTokenResponse MtlsTokenProviderPrivate::operator()(
    client::CancellationContext& context) const {
  const auto response = GetResponse(context);
  return response ? client::OauthTokenResponse(
                        {response.GetResult().GetAccessToken(),
                         response.GetResult().GetExpiryTime()})
                  : client::OauthTokenResponse(response.GetError());
}

ErrorResponse MtlsTokenProviderPrivate::GetErrorResponse() const {
  client::CancellationContext context;
  const auto response = GetResponse(context);
  if (response) {
    return ErrorResponse{};
  }

  ErrorResponse error_response;
  error_response.message = response.GetError().GetMessage();
  return error_response;
}

int MtlsTokenProviderPrivate::GetHttpStatusCode() const {
  client::CancellationContext context;
  const auto response = GetResponse(context);
  return response ? http::HttpStatusCode::OK
                  : response.GetError().GetHttpStatusCode();
}

bool MtlsTokenProviderPrivate::IsTokenResponseOK() const {
  client::CancellationContext context;
  return GetResponse(context).IsSuccessful();
}

AuthenticationSettings MtlsTokenProviderPrivate::MakeAuthenticationSettings(
    const MtlsSettings& settings) {
  AuthenticationSettings auth_settings;
  auth_settings.token_endpoint_url = settings.token_endpoint_url;
  auth_settings.retry_settings = settings.retry_settings;
  auth_settings.network_proxy_settings = settings.network_proxy_settings;
  return auth_settings;
}

bool MtlsTokenProviderPrivate::ShouldRefreshNow() const {
  return minimum_validity_ <= std::chrono::seconds(0) ||
         std::chrono::steady_clock::now() >= token_refresh_time_;
}

SignInClientResponse MtlsTokenProviderPrivate::GetResponse(
    client::CancellationContext& context) const {
  std::lock_guard<std::mutex> lock(request_mutex_);

  if (!ShouldRefreshNow()) {
    return current_token_;
  }

  if (context.IsCancelled()) {
    return SignInClientResponse(client::ApiError::Cancelled());
  }

  auto promise = std::make_shared<std::promise<SignInClientResponse>>();
  auto future = promise->get_future();
  auto auth_client = client_;
  auto properties = mtls_properties_;

  if (!context.ExecuteOrCancelled([&, auth_client]() {
        return auth_client->SignInMtls(
            properties, [promise](SignInClientResponse response) {
              promise->set_value(std::move(response));
            });
      })) {
    return SignInClientResponse(client::ApiError::Cancelled());
  }

  auto sign_in_response = future.get();
  if (context.IsCancelled()) {
    return SignInClientResponse(client::ApiError::Cancelled());
  }

  // `SignInMtls` reports any HTTP status >= 0 (including e.g. 401/403 for a
  // rejected client certificate) as a "successful" `Response`, since
  // `ParseAuthResponse` always builds a valid `SignInResult` regardless of
  // status code. An empty access token is the actual signal that the
  // request failed; convert it to a real `client::ApiError` here (mirrors
  // `TokenEndpointImpl::RequestToken(CancellationContext&, ...)`), so that
  // `current_token_` never caches a bogus "success" and every accessor
  // built on top of it (`operator()`, `operator bool`, `GetErrorResponse`,
  // `GetHttpStatusCode`) sees the failure.
  if (sign_in_response &&
      sign_in_response.GetResult().GetAccessToken().empty()) {
    const auto& sign_in_result = sign_in_response.GetResult();
    auto message = sign_in_result.GetFullMessage();
    if (message.empty()) {
      message = sign_in_result.GetErrorResponse().message;
    }
    sign_in_response =
        client::ApiError{sign_in_result.GetStatus(), std::move(message)};
  }

  current_token_ = std::move(sign_in_response);
  token_refresh_time_ = ComputeRefreshTime(current_token_, minimum_validity_);

  return current_token_;
}

MtlsTokenProviderImpl::MtlsTokenProviderImpl(
    MtlsSettings settings, std::chrono::seconds minimum_validity)
    : impl_(std::make_shared<MtlsTokenProviderPrivate>(std::move(settings),
                                                       minimum_validity)) {}

client::OauthTokenResponse MtlsTokenProviderImpl::operator()(
    client::CancellationContext& context) const {
  return impl_->operator()(context);
}

ErrorResponse MtlsTokenProviderImpl::GetErrorResponse() const {
  return impl_->GetErrorResponse();
}

int MtlsTokenProviderImpl::GetHttpStatusCode() const {
  return impl_->GetHttpStatusCode();
}

bool MtlsTokenProviderImpl::IsTokenResponseOK() const {
  return impl_->IsTokenResponseOK();
}

}  // namespace internal
}  // namespace authentication
}  // namespace olp
