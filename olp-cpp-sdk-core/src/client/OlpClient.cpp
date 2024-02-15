/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include "PendingUrlRequests.h"
#include "olp/core/client/Condition.h"
#include "olp/core/client/ErrorCode.h"
#include "olp/core/http/HttpStatusCode.h"
#include "olp/core/http/NetworkConstants.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/shared_mutex.h"
#include "olp/core/thread/Atomic.h"
#include "olp/core/utils/Url.h"

namespace {
constexpr auto kLogTag = "OlpClient";
constexpr auto kApiKeyParam = "apiKey=";
constexpr auto kHttpPrefix = "http://";
constexpr auto kHttpsPrefix = "https://";

struct RequestSettings {
  explicit RequestSettings(const int initial_backdown_period_ms,
                           const int timeout_s)
      : current_backdown_period{initial_backdown_period_ms},
        max_wait_time{std::chrono::seconds(timeout_s)} {}

  int current_try{0};
  std::chrono::milliseconds accumulated_wait_time{0};
  std::chrono::milliseconds current_backdown_period{0};
  const std::chrono::milliseconds max_wait_time{0};
};
}  // namespace

namespace olp {
namespace client {
namespace {

using PendingUrlRequestsPtr = std::shared_ptr<PendingUrlRequests>;
using PendingUrlRequestPtr = PendingUrlRequests::PendingUrlRequestPtr;
using NetworkRequestPtr = std::shared_ptr<http::NetworkRequest>;
using RequestSettingsPtr = std::shared_ptr<RequestSettings>;

using NetworkCallbackType = std::function<void(const http::RequestId request_id,
                                               HttpResponse response)>;

static const auto kCancelledErrorResponse =
    http::NetworkResponse()
        .WithStatus(static_cast<int>(http::ErrorCode::CANCELLED_ERROR))
        .WithError("Operation Cancelled.");

static const auto kTimeoutErrorResponse =
    http::NetworkResponse()
        .WithStatus(static_cast<int>(http::ErrorCode::TIMEOUT_ERROR))
        .WithError("Network request timed out.");

NetworkStatistics GetStatistics(const http::NetworkResponse& response) {
  return NetworkStatistics(response.GetBytesUploaded(),
                           response.GetBytesDownloaded());
}

HttpResponse ToHttpResponse(const http::NetworkResponse& response) {
  HttpResponse http_response{response.GetStatus(), response.GetError()};
  http_response.SetNetworkStatistics(GetStatistics(response));
  return http_response;
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

RequestSettingsPtr GetRequestSettings(const RetrySettings& retry_settings) {
  return std::make_shared<RequestSettings>(
      retry_settings.initial_backdown_period, retry_settings.timeout);
}

bool CheckRetryCondition(const RequestSettings& request,
                         const RetrySettings& settings,
                         HttpResponse& response) {
  return (request.current_try > settings.max_attempts ||
          !settings.retry_condition(response) ||
          request.accumulated_wait_time >= request.max_wait_time);
}

std::chrono::milliseconds CalculateNextWaitTime(const RetrySettings& settings,
                                                size_t current_try) {
  return settings.backdown_strategy
             ? settings.backdown_strategy(
                   std::chrono::milliseconds(settings.initial_backdown_period),
                   current_try)
             : std::chrono::milliseconds::zero();
}

void ExecuteSingleRequest(const std::shared_ptr<http::Network>& network,
                          const PendingUrlRequestPtr& pending_request,
                          const http::NetworkRequest& request,
                          const NetworkCallbackType& callback) {
  auto response_body = std::make_shared<std::stringstream>();
  auto headers = std::make_shared<http::Headers>();

  auto make_request = [&](http::RequestId& id) {
    auto send_outcome = network->Send(
        request, response_body,
        [=](const http::NetworkResponse& response) {
          auto status = response.GetStatus();
          if (!StatusSuccess(status)) {
            response_body->str(
                response.GetError().empty()
                    ? "Error occurred, please check HTTP status code"
                    : response.GetError());
          }

          callback(response.GetRequestId(),
                   {status, std::move(*response_body), std::move(*headers)});
        },
        [=](std::string key, std::string value) {
          headers->emplace_back(std::move(key), std::move(value));
        });

    if (!send_outcome.IsSuccessful()) {
      callback(PendingUrlRequest::kInvalidRequestId,
               ToHttpResponse(send_outcome));
      return CancellationToken();
    }

    id = send_outcome.GetRequestId();
    return CancellationToken([=] { network->Cancel(id); });
  };

  auto cancelled_func = [&]() {
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "ExecuteSingleRequest - already cancelled, url='%s'",
                        request.GetUrl().c_str());
    callback(PendingUrlRequest::kInvalidRequestId,
             ToHttpResponse(kCancelledErrorResponse));
  };

  pending_request->ExecuteOrCancelled(make_request, cancelled_func);
}

NetworkCallbackType GetRetryCallback(
    bool merge, const RequestSettingsPtr& settings,
    const RetrySettings& retry_settings,
    const std::shared_ptr<http::Network>& network,
    const PendingUrlRequestsPtr& pending_requests,
    const PendingUrlRequestPtr& pending_request,
    const NetworkRequestPtr& request) {
  return [=](const http::RequestId request_id, HttpResponse response) mutable {
    ++settings->current_try;

    if (CheckRetryCondition(*settings, retry_settings, response)) {
      // Response is either successull or retries count/time expired
      if (pending_request->GetRequestId() != request_id) {
        OLP_SDK_LOG_WARNING_F(
            kLogTag,
            "Wrong response received, pending_request=%" PRIu64
            ", request_id=%" PRIu64,
            pending_request->GetRequestId(), request_id);
        return;
      }

      if (merge) {
        const auto& url = request->GetUrl();
        pending_requests->OnRequestCompleted(request_id, url,
                                             std::move(response));
      } else {
        pending_request->OnRequestCompleted(std::move(response));
      }
      return;
    }

    const auto start = std::chrono::steady_clock::now();

    // TODO: Do not block thread but instead implement an event queue that will
    // trigger next retry once the time expired!
    const auto actual_wait_time =
        std::min(settings->current_backdown_period,
                 settings->max_wait_time - settings->accumulated_wait_time);
    std::this_thread::sleep_for(actual_wait_time);

    settings->accumulated_wait_time += actual_wait_time;
    settings->current_backdown_period =
        CalculateNextWaitTime(retry_settings, settings->current_try);

    OLP_SDK_LOG_DEBUG(
        kLogTag, "retry_callback - retrigger after sleep, actual_wait_time="
                     << actual_wait_time.count() << "ms, slept="
                     << std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - start)
                            .count()
                     << "ms");

    ExecuteSingleRequest(
        network, pending_request, *request,
        GetRetryCallback(merge, settings, retry_settings, network,
                         pending_requests, pending_request, request));
  };
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
  } else if (verb.compare("HEAD") == 0) {
    return http::NetworkRequest::HttpVerb::HEAD;
  } else if (verb.compare("PATCH") == 0) {
    return http::NetworkRequest::HttpVerb::PATCH;
  }

  return http::NetworkRequest::HttpVerb::GET;
}

HttpResponse SendRequest(const http::NetworkRequest& request,
                         const http::Network::DataCallback& data_callback,
                         const olp::client::OlpClientSettings& settings,
                         const olp::client::RetrySettings& retry_settings,
                         client::CancellationContext context) {
  struct ResponseData {
    Condition condition;
    http::NetworkResponse response{kCancelledErrorResponse};
    http::Headers headers;
  };

  auto response_data = std::make_shared<ResponseData>();

  // We dont need a response body in case we want a stream
  auto response_body =
      data_callback ? nullptr : std::make_shared<std::stringstream>();

  http::SendOutcome outcome{http::ErrorCode::CANCELLED_ERROR};
  const auto timeout = std::chrono::seconds(retry_settings.timeout);

  context.ExecuteOrCancelled(
      [&]() {
        outcome = settings.network_request_handler->Send(
            request, response_body,
            [response_data](http::NetworkResponse response) {
              response_data->response = std::move(response);
              response_data->condition.Notify();
            },
            [response_data](std::string key, std::string value) {
              response_data->headers.emplace_back(std::move(key),
                                                  std::move(value));
            },
            data_callback);

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
          response_data->condition.Notify();
        });
      },
      [&]() { response_data->condition.Notify(); });

  if (!outcome.IsSuccessful()) {
    return ToHttpResponse(outcome);
  }

  if (!response_data->condition.Wait(timeout)) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "Request timed out, request_id=%" PRIu64
        ", timeout=%i, retry_count=%i, url='%s'",
        outcome.GetRequestId(), static_cast<int>(timeout.count()),
        retry_settings.max_attempts, request.GetUrl().c_str());
    context.CancelOperation();
    return ToHttpResponse(kTimeoutErrorResponse);
  }

  if (context.IsCancelled()) {
    return ToHttpResponse(kCancelledErrorResponse);
  }

  HttpResponse response = [&]() {
    const auto status = response_data->response.GetStatus();
    if (status < 0) {
      return HttpResponse{status, response_data->response.GetError()};
    } else if (response_body) {
      return HttpResponse{status, std::move(*response_body),
                          std::move(response_data->headers)};
    } else {
      return HttpResponse{status, std::stringstream(),
                          std::move(response_data->headers)};
    }
  }();

  response.SetNetworkStatistics(GetStatistics(response_data->response));

  return response;
}

bool IsPending(const PendingUrlRequestPtr& request) {
  // A request is pending when it already triggered a Network call
  return request &&
         request->GetRequestId() != PendingUrlRequest::kInvalidRequestId;
}

}  // namespace

class OlpClient::OlpClientImpl {
 public:
  OlpClientImpl();
  OlpClientImpl(const OlpClientSettings& settings, std::string base_url);
  ~OlpClientImpl() = default;

  void SetBaseUrl(const std::string& base_url);
  std::string GetBaseUrl() const;

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
                       ParametersType header_params,
                       http::Network::DataCallback data_callback,
                       RequestBodyType post_body, std::string content_type,
                       CancellationContext context) const;

  std::shared_ptr<http::NetworkRequest> CreateRequest(
      const std::string& path, const std::string& method,
      const ParametersType& query_params, const ParametersType& header_params,
      const RequestBodyType& post_body, const std::string& content_type) const;

  boost::optional<ApiError> AddBearer(bool query_empty,
                                      http::NetworkRequest& request,
                                      CancellationContext& context) const;

 private:
  using MutexType = std::shared_mutex;
  using ReadLock = std::shared_lock<MutexType>;

  thread::Atomic<std::string, MutexType, ReadLock> base_url_;
  ParametersType default_headers_;
  OlpClientSettings settings_;
  PendingUrlRequestsPtr pending_requests_;

  bool ValidateBaseUrl() const;
};

OlpClient::OlpClientImpl::OlpClientImpl()
    : pending_requests_{std::make_shared<PendingUrlRequests>()} {}

OlpClient::OlpClientImpl::OlpClientImpl(const OlpClientSettings& settings,
                                        std::string base_url)
    : base_url_{std::move(base_url)},
      settings_{settings},
      pending_requests_{std::make_shared<PendingUrlRequests>()} {}

void OlpClient::OlpClientImpl::SetBaseUrl(const std::string& base_url) {
  base_url_.lockedAssign(base_url);
}

std::string OlpClient::OlpClientImpl::GetBaseUrl() const {
  return base_url_.lockedCopy();
}

OlpClient::ParametersType&
OlpClient::OlpClientImpl::GetMutableDefaultHeaders() {
  return default_headers_;
}

void OlpClient::OlpClientImpl::SetSettings(const OlpClientSettings& settings) {
  // I would not expect that settings change during lifetime of the instance.
  settings_ = settings;
}

boost::optional<client::ApiError> OlpClient::OlpClientImpl::AddBearer(
    bool query_empty, http::NetworkRequest& request,
    CancellationContext& context) const {
  const auto& settings = settings_.authentication_settings;
  if (!settings) {
    return boost::none;
  }

  if (settings->api_key_provider) {
    const auto& api_key = settings->api_key_provider();
    request.WithUrl(request.GetUrl() + (query_empty ? "?" : "&") +
                    kApiKeyParam + api_key);
    return boost::none;
  }

  std::string token;

  if (settings->token_provider) {
    auto response = settings->token_provider(context);
    if (!response) {
      return response.GetError();
    }

    token = response.GetResult().GetAccessToken();
  } else {
    // There is no token provider defined.
    return boost::none;
  }

  if (token.empty()) {
    return client::ApiError(
        static_cast<int>(http::ErrorCode::AUTHORIZATION_ERROR),
        "Invalid bearer token.");
  }

  request.WithHeader(http::kAuthorizationHeader,
                     http::kBearer + std::string(" ") + token);
  return boost::none;
}

bool OlpClient::OlpClientImpl::ValidateBaseUrl() const {
  return base_url_.locked([](const std::string& base_url) {
    return base_url == "" || (base_url.find(kHttpPrefix) != std::string::npos ||
                              base_url.find(kHttpsPrefix) != std::string::npos);
  });
}

std::shared_ptr<http::NetworkRequest> OlpClient::OlpClientImpl::CreateRequest(
    const std::string& path, const std::string& method,
    const OlpClient::ParametersType& query_params,
    const OlpClient::ParametersType& header_params,
    const RequestBodyType& post_body, const std::string& content_type) const {
  auto network_request = std::make_shared<http::NetworkRequest>(
      utils::Url::Construct(GetBaseUrl(), path, query_params));

  network_request->WithVerb(GetHttpVerb(method));

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
  if (!settings_.network_request_handler) {
    callback({static_cast<int>(http::ErrorCode::OFFLINE_ERROR),
              "Network layer offline or missing."});
    return CancellationToken();
  }

  if (!ValidateBaseUrl()) {
    callback({static_cast<int>(http::ErrorCode::INVALID_URL_ERROR),
              "Base URI does not contain a protocol"});
    return CancellationToken();
  }

  auto network_request = CreateRequest(path, method, query_params,
                                       header_params, post_body, content_type);

  CancellationContext context;
  const auto optional_error =
      AddBearer(query_params.empty(), *network_request, context);
  if (optional_error) {
    auto status = optional_error->GetHttpStatusCode();
    if (status == static_cast<int>(http::ErrorCode::UNKNOWN_ERROR)) {
      status = static_cast<int>(optional_error->GetErrorCode());
    }

    callback({status, optional_error->GetMessage()});
    return CancellationToken();
  }

  PendingUrlRequestPtr request_ptr = nullptr;
  auto& pending_requests = pending_requests_;
  const auto& url = network_request->GetUrl();
  CancellationToken cancellation_token;

  // Only merge same request in case there is no body as a body can alter the
  // outcome of the request and may not match the response of a request with a
  // different body.
  bool merge = (!post_body || post_body->empty());
  OLP_SDK_LOG_DEBUG_F(kLogTag, "CallApi: url='%s', merge='%s'", url.c_str(),
                      merge ? "true" : "false");

  if (merge) {
    // Add callback and prepare CancellationToken
    auto call_id = pending_requests->Append(url, callback, request_ptr);
    cancellation_token =
        CancellationToken([=] { pending_requests->Cancel(url, call_id); });

    if (IsPending(request_ptr)) {
      // Network call is already triggered, we only need to append our
      // callback Cancels this callback; internally once all pending callbacks
      // are cancelled the Network call will be automatically cancelled also
      return cancellation_token;
    }
  } else {
    // In case we do not merge same requests create a single PendingUrlRequest
    // as it is handling the cancellation the same as we would do normally
    request_ptr = std::make_shared<PendingUrlRequest>();

    // Add callback and prepare CancellationToken
    auto call_id = request_ptr->Append(callback);
    cancellation_token =
        CancellationToken([=] { request_ptr->Cancel(call_id); });
  }

  const auto& retry_settings = settings_.retry_settings;
  auto proxy = settings_.proxy_settings.value_or(http::NetworkProxySettings());
  network_request->WithSettings(
      http::NetworkSettings()
          .WithConnectionTimeout(retry_settings.connection_timeout)
          .WithTransferTimeout(retry_settings.transfer_timeout)
          .WithProxySettings(std::move(proxy)));

  auto network = settings_.network_request_handler;
  auto request_settings = GetRequestSettings(retry_settings);

  ExecuteSingleRequest(
      network, request_ptr, *network_request,
      GetRetryCallback(merge, request_settings, retry_settings, network,
                       pending_requests, request_ptr, network_request));

  return cancellation_token;
}

HttpResponse OlpClient::OlpClientImpl::CallApi(
    std::string path, std::string method,
    OlpClient::ParametersType query_params,
    OlpClient::ParametersType header_params,
    http::Network::DataCallback data_callback,
    OlpClient::RequestBodyType post_body, std::string content_type,
    CancellationContext context) const {
  if (!settings_.network_request_handler) {
    return HttpResponse(static_cast<int>(olp::http::ErrorCode::OFFLINE_ERROR),
                        "Network request handler is empty.");
  }

  if (!ValidateBaseUrl()) {
    return HttpResponse(
        static_cast<int>(olp::http::ErrorCode::INVALID_URL_ERROR),
        "Base URI does not contain a protocol");
  }

  const auto& retry_settings = settings_.retry_settings;
  auto network_settings =
      http::NetworkSettings()
          .WithTransferTimeout(retry_settings.transfer_timeout)
          .WithConnectionTimeout(retry_settings.connection_timeout)
          .WithProxySettings(
              settings_.proxy_settings.value_or(http::NetworkProxySettings()));

  auto network_request = http::NetworkRequest(
      utils::Url::Construct(GetBaseUrl(), path, query_params));

  network_request.WithVerb(GetHttpVerb(method))
      .WithBody(std::move(post_body))
      .WithSettings(std::move(network_settings));

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

  const auto optional_error =
      AddBearer(query_params.empty(), network_request, context);
  if (optional_error) {
    auto status = optional_error->GetHttpStatusCode();
    if (status == static_cast<int>(http::ErrorCode::UNKNOWN_ERROR)) {
      status = static_cast<int>(optional_error->GetErrorCode());
    }

    return {status, optional_error->GetMessage()};
  }

  auto response = SendRequest(network_request, data_callback, settings_,
                              retry_settings, context);

  NetworkStatistics accumulated_statistics = response.GetNetworkStatistics();

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

    backdown_period = CalculateNextWaitTime(retry_settings, i);
    response = SendRequest(network_request, data_callback, settings_,
                           retry_settings, context);

    // In case we retry, accumulate the stats
    accumulated_statistics += response.GetNetworkStatistics();
  }

  response.SetNetworkStatistics(accumulated_statistics);

  return response;
}

OlpClient::OlpClient() : impl_(std::make_shared<OlpClientImpl>()) {}
OlpClient::OlpClient(const OlpClientSettings& settings, std::string base_url)
    : impl_(std::make_shared<OlpClientImpl>(settings, std::move(base_url))) {}
OlpClient::~OlpClient() = default;
OlpClient::OlpClient(const OlpClient&) = default;
OlpClient& OlpClient::operator=(const OlpClient&) = default;
OlpClient::OlpClient(OlpClient&&) noexcept = default;
OlpClient& OlpClient::operator=(OlpClient&&) noexcept = default;

void OlpClient::SetBaseUrl(const std::string& base_url) {
  impl_->SetBaseUrl(base_url);
}

std::string OlpClient::GetBaseUrl() const { return impl_->GetBaseUrl(); }

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
                                ParametersType /*form_params*/,
                                RequestBodyType post_body,
                                std::string content_type,
                                CancellationContext context) const {
  return impl_->CallApi(std::move(path), std::move(method),
                        std::move(query_params), std::move(header_params),
                        nullptr, std::move(post_body), std::move(content_type),
                        std::move(context));
}

HttpResponse OlpClient::CallApiStream(std::string path, std::string method,
                                      ParametersType query_params,
                                      ParametersType header_params,
                                      http::Network::DataCallback data_callback,
                                      RequestBodyType post_body,
                                      std::string content_type,
                                      CancellationContext context) const {
  return impl_->CallApi(std::move(path), std::move(method),
                        std::move(query_params), std::move(header_params),
                        std::move(data_callback), std::move(post_body),
                        std::move(content_type), std::move(context));
}

}  // namespace client
}  // namespace olp
