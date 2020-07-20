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

#include "olp/core/client/OlpClient.h"

#include <cctype>
#include <chrono>
#include <future>
#include <sstream>
#include <thread>

#include "olp/core/client/Condition.h"
#include "olp/core/client/ErrorCode.h"
#include "olp/core/http/HttpStatusCode.h"
#include "olp/core/http/NetworkConstants.h"
#include "olp/core/logging/Log.h"
#include "olp/core/utils/Url.h"

namespace {
constexpr auto kLogTag = "OlpClient";
}  // namespace

namespace olp {
namespace client {

namespace {
static const auto kCancelledErrorResponse =
    http::NetworkResponse()
        .WithStatus(static_cast<int>(http::ErrorCode::CANCELLED_ERROR))
        .WithError("Operation Cancelled.");

static const auto kTimeoutErrorResponse =
    http::NetworkResponse()
        .WithStatus(static_cast<int>(http::ErrorCode::TIMEOUT_ERROR))
        .WithError("Network request timed out.");

HttpResponse ToHttpResponse(const http::NetworkResponse& response) {
  return {response.GetStatus(), response.GetError()};
}

HttpResponse ToHttpResponse(const http::SendOutcome& outcome) {
  return {static_cast<int>(outcome.GetErrorCode()),
          http::ErrorCodeToString(outcome.GetErrorCode())};
}

bool StatusSuccess(int status) {
  return status >= 0 && status < http::HttpStatusCode::BAD_REQUEST;
}

bool CaseInsensitiveCompare(const std::string& str1, const std::string& str2) {
  return (str1.size() == str2.size()) &&
         std::equal(str1.begin(), str1.end(), str2.begin(),
                    [](const char& c1, const char& c2) {
                      return (c1 == c2) ||
                             (std::tolower(c1) == std::tolower(c2));
                    });
}

void AddBearer(const boost::optional<AuthenticationSettings>& settings,
               http::NetworkRequest& request) {
  if (settings && settings.get().provider) {
    std::string bearer =
        http::kBearer + std::string(" ") + settings.get().provider();
    request.WithHeader(http::kAuthorizationHeader, bearer);
  }
}

CancellationToken ExecuteSingleRequest(
    std::weak_ptr<http::Network> weak_network,
    const http::NetworkRequest& request, const NetworkAsyncCallback& callback) {
  auto response_body = std::make_shared<std::stringstream>();
  auto headers = std::make_shared<http::Headers>();
  auto network = weak_network.lock();

  if (!network) {
    callback({static_cast<int>(http::ErrorCode::OFFLINE_ERROR),
              "Network layer offline or missing."});
    return CancellationToken();
  }

  auto send_outcome = network->Send(
      request, response_body,
      [=](const http::NetworkResponse& response) {
        int status = response.GetStatus();
        if (!StatusSuccess(status)) {
          response_body->str(
              response.GetError().empty()
                  ? "Error occurred. Please check HTTP status code"
                  : response.GetError());
        }

        callback({status, std::move(*response_body), std::move(*headers)});
      },
      [headers](std::string key, std::string value) {
        headers->emplace_back(std::move(key), std::move(value));
      });

  if (!send_outcome.IsSuccessful()) {
    callback(ToHttpResponse(send_outcome));
    return CancellationToken();
  }

  auto request_id = send_outcome.GetRequestId();

  return CancellationToken([weak_network, request_id]() {
    auto network = weak_network.lock();
    if (network) {
      network->Cancel(request_id);
    }
  });
}

http::NetworkRequest::HttpVerb GetHttpVerb(const std::string& verb) {
  if (verb.compare("GET") == 0) {
    return http::NetworkRequest::HttpVerb::GET;
  } else if (verb.compare("PUT") == 0) {
    return http::NetworkRequest::HttpVerb::PUT;
  } else if (verb.compare("POST") == 0) {
    return http::NetworkRequest::HttpVerb::POST;
  } else if (verb.compare("DELETE") == 0) {
    return http::NetworkRequest::HttpVerb::DEL;
  } else if (verb.compare("OPTIONS") == 0) {
    return http::NetworkRequest::HttpVerb::OPTIONS;
  }

  return http::NetworkRequest::HttpVerb::GET;
}

HttpResponse SendRequest(const http::NetworkRequest& request,
                         const olp::client::OlpClientSettings& settings,
                         const olp::client::RetrySettings& retry_settings,
                         client::CancellationContext context) {
  struct ResponseData {
    Condition condition_;
    http::NetworkResponse response_{kCancelledErrorResponse};
    http::Headers headers_;
  };

  auto response_data = std::make_shared<ResponseData>();
  auto response_body = std::make_shared<std::stringstream>();
  http::SendOutcome outcome{http::ErrorCode::CANCELLED_ERROR};
  const auto timeout = std::chrono::seconds(retry_settings.timeout);

  context.ExecuteOrCancelled(
      [&]() {
        outcome = settings.network_request_handler->Send(
            request, response_body,
            [response_data](http::NetworkResponse response) {
              response_data->response_ = std::move(response);
              response_data->condition_.Notify();
            },
            [response_data](std::string key, std::string value) {
              response_data->headers_.emplace_back(std::move(key),
                                                   std::move(value));
            });

        if (!outcome.IsSuccessful()) {
          OLP_SDK_LOG_WARNING_F(kLogTag,
                                "SendRequest: sending request failed, url=%s",
                                request.GetUrl().c_str());
          return CancellationToken();
        }

        auto request_handler = settings.network_request_handler;
        auto request_id = outcome.GetRequestId();

        return CancellationToken([=]() {
          request_handler->Cancel(request_id);
          response_data->condition_.Notify();
        });
      },
      [&]() { response_data->condition_.Notify(); });

  if (!outcome.IsSuccessful()) {
    return ToHttpResponse(outcome);
  }

  if (!response_data->condition_.Wait(timeout)) {
    OLP_SDK_LOG_WARNING_F(kLogTag, "Request %" PRIu64 " timed out!",
                          outcome.GetRequestId());
    context.CancelOperation();
    return ToHttpResponse(kTimeoutErrorResponse);
  }

  if (context.IsCancelled()) {
    return ToHttpResponse(kCancelledErrorResponse);
  }

  return {response_data->response_.GetStatus(), std::move(*response_body),
          std::move(response_data->headers_)};
}

std::chrono::milliseconds CalculateNextWaitTime(
    const RetrySettings& settings,
    std::chrono::milliseconds current_backdown_period, size_t current_try) {
  if (auto backdown_strategy = settings.backdown_strategy) {
    return backdown_strategy(
        std::chrono::milliseconds(settings.initial_backdown_period),
        current_try);
  } else if (auto backdown_policy = settings.backdown_policy) {
    return std::chrono::milliseconds(
        backdown_policy(static_cast<int>(current_backdown_period.count())));
  }
  return std::chrono::milliseconds::zero();
}

NetworkAsyncCallback GetRetryCallback(
    int current_try, std::chrono::milliseconds current_backdown_period,
    std::chrono::milliseconds accumulated_wait_time,
    const RetrySettings& settings, const NetworkAsyncCallback& callback,
    const std::shared_ptr<http::NetworkRequest>& network_request,
    std::weak_ptr<http::Network> network,
    const std::weak_ptr<CancellationContext>& weak_cancel_context) {
  ++current_try;
  return [=](HttpResponse response) {
    const auto max_wait_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::seconds(settings.timeout));

    if (current_try > settings.max_attempts ||
        !settings.retry_condition(response) ||
        accumulated_wait_time >= max_wait_time) {
      // Response is either successull or retries count/time expired
      callback(std::move(response));
      return;
    }

    // TODO there must be a better way
    const auto actual_wait_time = std::min(
        current_backdown_period, max_wait_time - accumulated_wait_time);
    std::this_thread::sleep_for(actual_wait_time);

    const auto next_wait_time =
        CalculateNextWaitTime(settings, current_backdown_period, current_try);

    auto cancel_context = weak_cancel_context.lock();
    if (cancel_context) {
      cancel_context->ExecuteOrCancelled(
          [&]() -> CancellationToken {
            return ExecuteSingleRequest(
                network, *network_request,
                GetRetryCallback(current_try, next_wait_time,
                                 accumulated_wait_time + actual_wait_time,
                                 settings, callback, network_request, network,
                                 weak_cancel_context));
          },
          [callback] { callback(ToHttpResponse(kCancelledErrorResponse)); });
    } else {
      // Last (and only) strong reference lives in cancellation
      // token, which is reset on CancelOperation.
      callback(ToHttpResponse(kCancelledErrorResponse));
    }
  };
}

}  // namespace

class OlpClient::OlpClientImpl {
 public:
  OlpClientImpl() = default;
  ~OlpClientImpl() = default;

  void SetBaseUrl(const std::string& base_url);
  const std::string& GetBaseUrl() const;

  ParametersType& GetMutableDefaultHeaders();
  void SetSettings(const OlpClientSettings& settings);

  CancellationToken CallApi(const std::string& path, const std::string& method,
                            const ParametersType& query_params,
                            const ParametersType& header_params,
                            const ParametersType& form_params,
                            const RequestBodyType& post_body,
                            const std::string& content_type,
                            const NetworkAsyncCallback& callback) const;

  HttpResponse CallApi(std::string path, std::string method,
                       ParametersType query_params,
                       ParametersType header_params, ParametersType form_params,
                       RequestBodyType post_body, std::string content_type,
                       CancellationContext context) const;

  std::shared_ptr<http::NetworkRequest> CreateRequest(
      const std::string& path, const std::string& method,
      const ParametersType& query_params, const ParametersType& header_params,
      const RequestBodyType& post_body, const std::string& content_type) const;

 private:
  std::string base_url_;
  ParametersType default_headers_;
  OlpClientSettings settings_;
};

void OlpClient::OlpClientImpl::SetBaseUrl(const std::string& base_url) {
  base_url_ = base_url;
}

const std::string& OlpClient::OlpClientImpl::GetBaseUrl() const {
  return base_url_;
}

OlpClient::ParametersType&
OlpClient::OlpClientImpl::GetMutableDefaultHeaders() {
  return default_headers_;
}

void OlpClient::OlpClientImpl::SetSettings(const OlpClientSettings& settings) {
  settings_ = settings;
}

std::shared_ptr<http::NetworkRequest> OlpClient::OlpClientImpl::CreateRequest(
    const std::string& path, const std::string& method,
    const OlpClient::ParametersType& query_params,
    const OlpClient::ParametersType& header_params,
    const RequestBodyType& post_body, const std::string& content_type) const {
  auto network_request = std::make_shared<http::NetworkRequest>(
      utils::Url::Construct(base_url_, path, query_params));

  network_request->WithVerb(GetHttpVerb(method));

  AddBearer(settings_.authentication_settings, *network_request);

  for (const auto& header : default_headers_) {
    network_request->WithHeader(header.first, header.second);
  }

  std::string custom_user_agent;
  for (const auto& header : header_params) {
    // Merge all User-Agent headers into one header.
    // This is required for (at least) iOS network implementation,
    // which uses headers dictionary without any duplicates.
    // User agents entries are usually separated by a whitespace, e.g.
    // Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:47.0) Firefox/47.0
    if (CaseInsensitiveCompare(header.first, http::kUserAgentHeader)) {
      custom_user_agent += header.second + std::string(" ");
    } else {
      network_request->WithHeader(header.first, header.second);
    }
  }

  custom_user_agent += http::kOlpSdkUserAgent;
  network_request->WithHeader(http::kUserAgentHeader, custom_user_agent);

  if (!content_type.empty()) {
    network_request->WithHeader(http::kContentTypeHeader, content_type);
  }

  network_request->WithBody(post_body);
  return network_request;
}

CancellationToken OlpClient::OlpClientImpl::CallApi(
    const std::string& path, const std::string& method,
    const OlpClient::ParametersType& query_params,
    const OlpClient::ParametersType& header_params,
    const OlpClient::ParametersType& /*form_params*/,
    const OlpClient::RequestBodyType& post_body,
    const std::string& content_type,
    const NetworkAsyncCallback& callback) const {
  auto network_request = CreateRequest(path, method, query_params,
                                       header_params, post_body, content_type);

  const auto& retry_settings = settings_.retry_settings;

  auto network_settings =
      http::NetworkSettings()
          .WithConnectionTimeout(retry_settings.timeout)
          .WithTransferTimeout(retry_settings.timeout)
          .WithRetries(retry_settings.max_attempts)
          .WithProxySettings(
              settings_.proxy_settings.value_or(http::NetworkProxySettings()));

  network_request->WithSettings(std::move(network_settings));

  auto cancel_context = std::make_shared<CancellationContext>();
  std::weak_ptr<olp::http::Network> network = settings_.network_request_handler;

  auto retry_callback = GetRetryCallback(
      0, std::chrono::milliseconds(retry_settings.initial_backdown_period),
      std::chrono::milliseconds::zero(), retry_settings, callback,
      network_request, network, cancel_context);

  cancel_context->ExecuteOrCancelled(
      [=]() -> CancellationToken {
        return ExecuteSingleRequest(network, *network_request, retry_callback);
      },
      [callback] { callback(ToHttpResponse(kCancelledErrorResponse)); });

  return CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
}

HttpResponse OlpClient::OlpClientImpl::CallApi(
    std::string path, std::string method,
    OlpClient::ParametersType query_params,
    OlpClient::ParametersType header_params,
    OlpClient::ParametersType /*forms_params*/,
    OlpClient::RequestBodyType post_body, std::string content_type,
    CancellationContext context) const {
  if (!settings_.network_request_handler) {
    return HttpResponse(static_cast<int>(olp::http::ErrorCode::OFFLINE_ERROR),
                        "Network request handler is empty.");
  }

  const auto& retry_settings = settings_.retry_settings;
  auto network_settings =
      http::NetworkSettings()
          .WithTransferTimeout(retry_settings.timeout)
          .WithConnectionTimeout(retry_settings.timeout)
          .WithProxySettings(
              settings_.proxy_settings.value_or(http::NetworkProxySettings()));

  auto network_request =
      http::NetworkRequest(utils::Url::Construct(base_url_, path, query_params))
          .WithVerb(GetHttpVerb(method))
          .WithBody(std::move(post_body))
          .WithSettings(std::move(network_settings));

  AddBearer(settings_.authentication_settings, network_request);

  for (const auto& header : default_headers_) {
    network_request.WithHeader(header.first, header.second);
  }

  std::string custom_user_agent;
  for (const auto& header : header_params) {
    // Merge all User-Agent headers into one header.
    // This is required for (at least) iOS network implementation,
    // which uses headers dictionary without any duplicates.
    // User agents entries are usually separated by a whitespace, e.g.
    // Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:47.0) Firefox/47.0
    if (CaseInsensitiveCompare(header.first, http::kUserAgentHeader)) {
      custom_user_agent += header.second + std::string(" ");
    } else {
      network_request.WithHeader(header.first, header.second);
    }
  }

  custom_user_agent += http::kOlpSdkUserAgent;
  network_request.WithHeader(http::kUserAgentHeader, custom_user_agent);

  if (!content_type.empty()) {
    network_request.WithHeader(http::kContentTypeHeader, content_type);
  }

  auto backdown_period =
      std::chrono::milliseconds(retry_settings.initial_backdown_period);

  auto response =
      SendRequest(network_request, settings_, retry_settings, context);

  // Make sure that we don't wait longer than `timeout` in retry settings
  auto accumulated_wait_time = backdown_period;
  const auto max_wait_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::seconds(retry_settings.timeout));
  for (int i = 1; i <= retry_settings.max_attempts && !context.IsCancelled() &&
                  accumulated_wait_time < max_wait_time;
       i++) {
    if (StatusSuccess(response.status)) {
      return response;
    }

    if (!retry_settings.retry_condition(response)) {
      return response;
    }

    // do the periodical sleep and check for cancellation status in between.
    auto duration_to_sleep =
        std::min(backdown_period, max_wait_time - accumulated_wait_time);
    accumulated_wait_time += duration_to_sleep;

    while (duration_to_sleep.count() > 0 && !context.IsCancelled()) {
      const auto sleep_ms =
          std::min(std::chrono::milliseconds(1000), duration_to_sleep);
      std::this_thread::sleep_for(sleep_ms);
      duration_to_sleep -= sleep_ms;
    }

    backdown_period = CalculateNextWaitTime(retry_settings, backdown_period, i);
    response = SendRequest(network_request, settings_, retry_settings, context);
  }

  return response;
}

OlpClient::OlpClient() : impl_(std::make_shared<OlpClientImpl>()){};
OlpClient::~OlpClient() = default;

OlpClient::OlpClient(const OlpClient&) = default;
OlpClient& OlpClient::operator=(const OlpClient&) = default;
OlpClient::OlpClient(OlpClient&&) noexcept = default;
OlpClient& OlpClient::operator=(OlpClient&&) noexcept = default;

void OlpClient::SetBaseUrl(const std::string& base_url) {
  impl_->SetBaseUrl(base_url);
}

const std::string& OlpClient::GetBaseUrl() const { return impl_->GetBaseUrl(); }

OlpClient::ParametersType& OlpClient::GetMutableDefaultHeaders() {
  return impl_->GetMutableDefaultHeaders();
}

void OlpClient::SetSettings(const OlpClientSettings& settings) {
  impl_->SetSettings(settings);
}

CancellationToken OlpClient::CallApi(
    const std::string& path, const std::string& method,
    const ParametersType& query_params, const ParametersType& header_params,
    const ParametersType& form_params, const RequestBodyType& post_body,
    const std::string& content_type,
    const NetworkAsyncCallback& callback) const {
  return impl_->CallApi(path, method, query_params, header_params, form_params,
                        post_body, content_type, callback);
}

HttpResponse OlpClient::CallApi(std::string path, std::string method,
                                ParametersType query_params,
                                ParametersType header_params,
                                ParametersType form_params,
                                RequestBodyType post_body,
                                std::string content_type,
                                CancellationContext context) const {
  return impl_->CallApi(std::move(path), std::move(method),
                        std::move(query_params), std::move(header_params),
                        std::move(form_params), std::move(post_body),
                        std::move(content_type), std::move(context));
}

}  // namespace client
}  // namespace olp
