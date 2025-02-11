/*
 * Copyright (C) 2021-2025 HERE Europe B.V.
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

#include "TokenEndpointImpl.h"

#include <memory>
#include <thread>
#include <utility>

#include <olp/authentication/SignInResult.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/NetworkConstants.h>
#include <olp/core/http/NetworkUtils.h>
#include <olp/core/logging/Log.h>
#include <olp/core/thread/Atomic.h>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "AuthenticationClientUtils.h"
#include "Constants.h"
#include "SignInResultImpl.h"

namespace olp {
namespace authentication {

namespace {
using TimeResponse = Response<time_t>;
using SignInResponse = Response<SignInResult>;
using RequestBodyData = client::OlpClient::RequestBodyType::element_type;

constexpr auto kApplicationJson = "application/json";
constexpr auto kOauthEndpoint = "/oauth2/token";
constexpr auto kTimestampEndpoint = "/timestamp";
constexpr auto kGrantType = "grantType";
constexpr auto kClientGrantType = "client_credentials";
constexpr auto kLogTag = "TokenEndpointImpl";
constexpr auto kErrorWrongTimestamp = 401204;
constexpr auto kScope = "scope";

std::string GetBasePath(const std::string& base_string) {
  // Remove /oauth2/token from url to make sure only the base url is used
  auto new_base_string = base_string;
  auto pos = new_base_string.find(kOauthEndpoint);
  if (pos != std::string::npos) {
    new_base_string.erase(pos, std::strlen(kOauthEndpoint));
  }

  OLP_SDK_LOG_INFO_F(
      kLogTag,
      "GetBasePath: old_token_endpoint_url='%s', token_endpoint_url='%s'",
      base_string.c_str(), new_base_string.c_str());

  return new_base_string;
}

AuthenticationSettings ConvertSettings(Settings settings) {
  AuthenticationSettings auth_settings;
  auth_settings.network_proxy_settings =
      std::move(settings.network_proxy_settings);
  auth_settings.network_request_handler =
      std::move(settings.network_request_handler);
  auth_settings.token_endpoint_url = GetBasePath(settings.token_endpoint_url);
  auth_settings.use_system_time = settings.use_system_time;
  auth_settings.retry_settings = std::move(settings.retry_settings);

  // Ignore `settings.task_scheduler`. It can cause a deadlock on the sign in
  // when used from another task within `TaskScheduler` with 1 thread.

  return auth_settings;
}

bool HasWrongTimestamp(const SignInResult& result) {
  const auto& error_response = result.GetErrorResponse();
  const auto status = result.GetStatus();
  return status == http::HttpStatusCode::UNAUTHORIZED &&
         error_response.code == kErrorWrongTimestamp;
}

void RetryDelay(const client::RetrySettings& retry_settings, size_t retry) {
  std::this_thread::sleep_for(retry_settings.backdown_strategy(
      std::chrono::milliseconds(retry_settings.initial_backdown_period),
      retry));
}

TimeResponse ParseTimeResponse(const std::string& payload) {
  boost::json::error_code ec;
  auto document = boost::json::parse(payload, ec);

  if (!document.is_object()) {
    return client::ApiError(client::ErrorCode::InternalFailure,
                            "JSON document root is not an Object type");
  }

  const auto timestamp_it = document.as_object().find("timestamp");
  if (timestamp_it == document.as_object().end() ||
      (!timestamp_it->value().is_uint64() &&
       !timestamp_it->value().is_int64())) {
    return client::ApiError(
        client::ErrorCode::InternalFailure,
        "JSON document must contain timestamp integer field");
  }

  return timestamp_it->value().to_number<uint64_t>();
}

std::string GenerateUid() {
  static thread::Atomic<boost::uuids::random_generator> generator;
  return generator.locked([](boost::uuids::random_generator& gen) {
    return boost::uuids::to_string(gen());
  });
}

client::OlpClient::RequestBodyType GenerateClientBody(
    const TokenRequest& token_request,
    const boost::optional<std::string>& scope) {
  boost::json::object object;

  object[kGrantType] = kClientGrantType;

  auto expires_in =
      static_cast<unsigned int>(token_request.GetExpiresIn().count());
  if (expires_in > 0) {
    object[Constants::EXPIRES_IN] = expires_in;
  }

  if (scope) {
    object[kScope] = scope.get();
  }

  auto content = boost::json::serialize(object);
  return std::make_shared<RequestBodyData>(content.begin(), content.end());
}

TimeResponse GetTimeFromServer(client::CancellationContext& context,
                               const client::OlpClient& client) {
  auto http_result = client.CallApi(kTimestampEndpoint, "GET", {}, {}, {},
                                    nullptr, {}, context);

  std::string response;
  http_result.GetResponse(response);

  if (http_result.GetStatus() != http::HttpStatusCode::OK) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag, "Failed to get time from server, status=%d, response='%s'",
        http_result.GetStatus(), response.c_str());

    return client::ApiError(http_result.GetStatus(), std::move(response));
  }

  auto server_time = ParseTimeResponse(response);
  if (!server_time) {
    const auto& error = server_time.GetError();
    const auto& message = error.GetMessage();
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "Failed to decode time from server, message='%s'",
                          message.c_str());
  }

  return server_time;
}

}  // namespace

TokenEndpointImpl::TokenEndpointImpl(Settings settings)
    : credentials_(std::move(settings.credentials)),
      scope_(std::move(settings.scope)),
      settings_(ConvertSettings(std::move(settings))),
      auth_client_(settings_) {}

client::CancellationToken TokenEndpointImpl::RequestToken(
    const TokenRequest& token_request, const RequestTokenCallback& callback) {
  AuthenticationClient::SignInProperties properties;
  properties.expires_in = token_request.GetExpiresIn();
  properties.scope = scope_;
  return auth_client_.SignInClient(
      credentials_, properties,
      [callback](
          const AuthenticationClient::SignInClientResponse& sign_in_response) {
        if (!sign_in_response) {
          callback(sign_in_response.GetError());
          return;
        }

        const auto& sign_in_result = sign_in_response.GetResult();
        if (sign_in_result.GetAccessToken().empty()) {
          callback(client::ApiError{sign_in_result.GetStatus(),
                                    sign_in_result.GetFullMessage()});
          return;
        }

        callback(TokenResult{
            sign_in_result.GetAccessToken(), sign_in_result.GetExpiresIn(),
            sign_in_result.GetScope().empty() ? boost::optional<std::string>{}
                                              : sign_in_result.GetScope()});
      });
}

std::future<TokenResponse> TokenEndpointImpl::RequestToken(
    client::CancellationToken& cancel_token,
    const TokenRequest& token_request) {
  auto promise = std::make_shared<std::promise<TokenResponse>>();
  cancel_token = RequestToken(token_request, [promise](TokenResponse response) {
    promise->set_value(std::move(response));
  });
  return promise->get_future();
}

TokenResponse TokenEndpointImpl::RequestToken(
    client::CancellationContext& context,
    const TokenRequest& token_request) const {
  auto sign_in_response = SignInClient(context, token_request);
  if (!sign_in_response) {
    return sign_in_response.GetError();
  }

  const auto& sign_in_result = sign_in_response.GetResult();
  if (sign_in_result.GetAccessToken().empty()) {
    auto message = sign_in_result.GetFullMessage();

    // Full message can be empty if an error occurred during response parsing.
    // Use message from an error response in this case.
    if (message.empty()) {
      message = sign_in_result.GetErrorResponse().message;
    }

    return client::ApiError{sign_in_result.GetStatus(), std::move(message)};
  }

  return TokenResult{
      sign_in_result.GetAccessToken(), sign_in_result.GetExpiresIn(),
      sign_in_result.GetScope().empty() ? boost::optional<std::string>{}
                                        : sign_in_result.GetScope()};
}

SignInResponse TokenEndpointImpl::SignInClient(
    client::CancellationContext& context,
    const TokenRequest& token_request) const {
  if (!settings_.network_request_handler) {
    return client::ApiError::NetworkConnection("Cannot sign in while offline");
  }

  if (context.IsCancelled()) {
    return client::ApiError::Cancelled();
  }

  auto client = CreateOlpClient(settings_, boost::none, false);

  RequestTimer timer = CreateRequestTimer(client, context);

  const auto request_body = GenerateClientBody(token_request, scope_);

  SignInResult response;

  const auto& retry_settings = settings_.retry_settings;

  for (auto retry = 0; retry < retry_settings.max_attempts; ++retry) {
    if (context.IsCancelled()) {
      return client::ApiError::Cancelled();
    }

    auto auth_response = CallAuth(client, kOauthEndpoint, context, request_body,
                                  timer.GetRequestTime());
    const auto status = auth_response.GetStatus();
    if (status < 0) {
      // If a timeout occurred, the cancellation id done through the context.
      // So this case needs to be handled independently of context state.
      if (status != static_cast<int>(http::ErrorCode::TIMEOUT_ERROR) &&
          context.IsCancelled()) {
        return client::ApiError::Cancelled();
      }

      // Auth response message may be empty in case of unknown errors.
      // Fill in the message as a status string representation in this case.
      auto message = auth_response.GetResponseAsString();
      if (message.empty()) {
        message = http::HttpErrorToString(status);
      }

      return client::ApiError(status, message);
    }

    response = ParseAuthResponse(status, auth_response.GetRawResponse());

    // The request ended up with `OK` status should not be retriggered even if
    // `retry_condition` is `true` for this `HttpResponse`.
    if (status == http::HttpStatusCode::OK) {
      break;
    }

    if (retry_settings.retry_condition(auth_response)) {
      RetryDelay(retry_settings, retry);
      continue;
    }

    // In case we can't authorize with system time, retry with the server
    // time from response headers (if available).
    if (HasWrongTimestamp(response)) {
      auto server_time = GetTimestampFromHeaders(auth_response.GetHeaders());
      if (server_time) {
        timer = RequestTimer(*server_time);
        continue;
      }
    }

    break;
  }

  return response;
}

SignInResult TokenEndpointImpl::ParseAuthResponse(
    int status, std::stringstream& auth_response) {
  boost::json::error_code ec;
  auto document = boost::json::parse(auth_response, ec);
  return std::make_shared<SignInResultImpl>(
      status, http::HttpErrorToString(status),
      ec.failed() || !document.is_object()
          ? nullptr
          : std::make_shared<boost::json::object>(document.as_object()));
}

client::HttpResponse TokenEndpointImpl::CallAuth(
    const client::OlpClient& client, const std::string& endpoint,
    client::CancellationContext& context,
    client::OlpClient::RequestBodyType body, std::time_t timestamp) const {
  const auto url = settings_.token_endpoint_url + endpoint;

  auto auth_header =
      GenerateAuthorizationHeader(credentials_, url, timestamp, GenerateUid());

  client::OlpClient::ParametersType headers = {
      {http::kAuthorizationHeader, std::move(auth_header)}};

  return client.CallApi(endpoint, "POST", {}, std::move(headers), {},
                        std::move(body), kApplicationJson, context);
}

TokenEndpointImpl::RequestTimer TokenEndpointImpl::CreateRequestTimer(
    const client::OlpClient& client,
    client::CancellationContext& context) const {
  if (settings_.use_system_time) {
    return RequestTimer();
  }

  auto server_time = GetTimeFromServer(context, client);
  if (!server_time) {
    return RequestTimer();
  }

  return RequestTimer(server_time.GetResult());
}

TokenEndpointImpl::RequestTimer::RequestTimer()
    : timer_start_{std::chrono::steady_clock::now()},
      time_{std::chrono::system_clock::to_time_t(
          std::chrono::system_clock::now())} {}

TokenEndpointImpl::RequestTimer::RequestTimer(std::time_t server_time)
    : timer_start_{std::chrono::steady_clock::now()}, time_{server_time} {}

std::time_t TokenEndpointImpl::RequestTimer::GetRequestTime() const {
  const auto now = std::chrono::steady_clock::now();
  const auto time_since_start =
      std::chrono::duration_cast<std::chrono::seconds>(now - timer_start_);
  return time_ + time_since_start.count();
}

}  // namespace authentication
}  // namespace olp
