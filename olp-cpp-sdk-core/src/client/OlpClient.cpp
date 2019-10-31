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

#include "olp/core/client/OlpClient.h"

#include <cctype>
#include <chrono>
#include <future>
#include <sstream>
#include <thread>

#include "olp/core/http/NetworkConstants.h"
#include "olp/core/utils/Url.h"

namespace olp {
namespace client {

namespace {
bool CaseInsensitiveCompare(const std::string& str1, const std::string& str2) {
  return (str1.size() == str2.size()) &&
         std::equal(str1.begin(), str1.end(), str2.begin(),
                    [](const char& c1, const char& c2) {
                      return (c1 == c2) ||
                             (std::tolower(c1) == std::tolower(c2));
                    });
}

std::string ErrorCodeToString(olp::http::ErrorCode code) {
  switch (code) {
    case olp::http::ErrorCode::SUCCESS:
      return "Success";
    case olp::http::ErrorCode::IO_ERROR:
      return "Input/Output error";
    case olp::http::ErrorCode::AUTHORIZATION_ERROR:
      return "Authorization error";
    case olp::http::ErrorCode::INVALID_URL_ERROR:
      return "Invalid URL";
    case olp::http::ErrorCode::OFFLINE_ERROR:
      return "Offline";
    case olp::http::ErrorCode::CANCELLED_ERROR:
      return "Cancelled";
    case olp::http::ErrorCode::AUTHENTICATION_ERROR:
      return "Authentication error";
    case olp::http::ErrorCode::TIMEOUT_ERROR:
      return "Timeout";
    case olp::http::ErrorCode::NETWORK_OVERLOAD_ERROR:
      return "Network overload";
    default:
      return "Unknown error";
  }
}

CancellationToken ExecuteSingleRequest(
    std::weak_ptr<http::Network> weak_network,
    const http::NetworkRequest& request, const NetworkAsyncCallback& callback) {
  auto response_body = std::make_shared<std::stringstream>();

  auto network = weak_network.lock();

  if (!network) {
    HttpResponse result(static_cast<int>(olp::http::ErrorCode::OFFLINE_ERROR));
    callback(std::move(result));
    return CancellationToken();
  }

  auto send_outcome = network->Send(
      request, response_body, [=](const http::NetworkResponse& response) {
        int status = response.GetStatus();
        if (status >= 400 || status < 0) {
          if (response.GetError().empty()) {
            response_body->str("Error occured. Please check HTTP status code.");
          } else {
            response_body->str(response.GetError());
          }
        }
        callback(HttpResponse(status, std::move(*response_body)));
      });

  if (!send_outcome.IsSuccessful()) {
    callback(HttpResponse(static_cast<int>(send_outcome.GetErrorCode()),
                          ErrorCodeToString(send_outcome.GetErrorCode())));
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
  http::NetworkRequest::HttpVerb http_verb =
      http::NetworkRequest::HttpVerb::GET;

  if (verb.compare("GET") == 0)
    http_verb = http::NetworkRequest::HttpVerb::GET;
  else if (verb.compare("PUT") == 0)
    http_verb = http::NetworkRequest::HttpVerb::PUT;
  else if (verb.compare("POST") == 0)
    http_verb = http::NetworkRequest::HttpVerb::POST;
  else if (verb.compare("DELETE") == 0)
    http_verb = http::NetworkRequest::HttpVerb::DEL;

  return http_verb;
}

}  // anonymous namespace

OlpClient::OlpClient() {}

void OlpClient::SetBaseUrl(const std::string& base_url) {
  base_url_ = base_url;
}

const std::string& OlpClient::GetBaseUrl() const { return base_url_; }

std::multimap<std::string, std::string>& OlpClient::GetMutableDefaultHeaders() {
  return default_headers_;
}

void OlpClient::SetSettings(const OlpClientSettings& settings) {
  settings_ = settings;
}

std::shared_ptr<http::NetworkRequest> OlpClient::CreateRequest(
    const std::string& path, const std::string& method,
    const std::multimap<std::string, std::string>& query_params,
    const std::multimap<std::string, std::string>& header_params,
    const std::shared_ptr<std::vector<unsigned char>>& post_body,
    const std::string& content_type) const {
  auto network_request = std::make_shared<http::NetworkRequest>(
      olp::utils::Url::Construct(base_url_, path, query_params));

  http::NetworkRequest::HttpVerb http_verb = GetHttpVerb(method);
  network_request->WithVerb(http_verb);

  if (settings_.authentication_settings) {
    std::string bearer = http::kBearer + std::string(" ") +
                         settings_.authentication_settings.get().provider();
    network_request->WithHeader(http::kAuthorizationHeader, bearer);
  }

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

NetworkAsyncCallback GetRetryCallback(
    int current_try, int current_backdown_period, const RetrySettings& settings,
    const NetworkAsyncCallback& callback,
    const std::shared_ptr<http::NetworkRequest>& network_request,
    std::weak_ptr<http::Network> network,
    const std::weak_ptr<CancellationContext>& weak_cancel_context) {
  ++current_try;
  return [=](HttpResponse response) {
    if (current_try >= settings.max_attempts ||
        !settings.retry_condition(response)) {
      callback(std::move(response));
    } else {
      // TODO there must be a better way
      std::this_thread::sleep_for(
          std::chrono::milliseconds(current_backdown_period));

      auto next_backdown_period =
          settings.backdown_policy(current_backdown_period);
      auto cancel_context = weak_cancel_context.lock();
      if (cancel_context) {
        cancel_context->ExecuteOrCancelled(
            [&]() -> CancellationToken {
              return ExecuteSingleRequest(
                  network, *network_request,
                  GetRetryCallback(current_try, next_backdown_period, settings,
                                   callback, network_request, network,
                                   weak_cancel_context));
            },
            [callback]() {
              callback(HttpResponse(
                  static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
                  "Operation Cancelled."));
            });
      } else {
        callback(HttpResponse(static_cast<int>(http::ErrorCode::UNKNOWN_ERROR),
                              "Unknown error."));
      }
    }
  };
}

CancellationToken OlpClient::CallApi(
    const std::string& path, const std::string& method,
    const std::multimap<std::string, std::string>& query_params,
    const std::multimap<std::string, std::string>& header_params,
    const std::multimap<std::string, std::string>& form_params,
    const std::shared_ptr<std::vector<unsigned char>>& post_body,
    const std::string& content_type,
    const NetworkAsyncCallback& callback) const {
  auto network_request = CreateRequest(path, method, query_params,
                                       header_params, post_body, content_type);

  auto proxy_settings = olp::http::NetworkProxySettings();

  if (this->settings_.proxy_settings) {
    proxy_settings = *this->settings_.proxy_settings;
  }
  olp::http::NetworkSettings network_settings;
  network_settings.WithConnectionTimeout(
      this->settings_.retry_settings.timeout);
  network_settings.WithTransferTimeout(this->settings_.retry_settings.timeout);
  network_settings.WithRetries(this->settings_.retry_settings.max_attempts);

  network_settings.WithProxySettings(std::move(proxy_settings));
  network_request->WithSettings(std::move(network_settings));

  auto cancel_context = std::make_shared<CancellationContext>();
  std::weak_ptr<olp::http::Network> network =
      this->settings_.network_request_handler;

  RetrySettings retry_settings;
  retry_settings.max_attempts = settings_.retry_settings.max_attempts;
  retry_settings.timeout = settings_.retry_settings.timeout;
  retry_settings.initial_backdown_period =
      settings_.retry_settings.initial_backdown_period;
  retry_settings.backdown_policy = settings_.retry_settings.backdown_policy;
  retry_settings.retry_condition = settings_.retry_settings.retry_condition;

  NetworkAsyncCallback retry_callback = GetRetryCallback(
      0, settings_.retry_settings.initial_backdown_period, retry_settings,
      callback, network_request, network, cancel_context);

  cancel_context->ExecuteOrCancelled(
      [=]() -> CancellationToken {
        return ExecuteSingleRequest(network, *network_request, retry_callback);
      },
      [callback]() {
        callback(
            HttpResponse(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
                         "Operation Cancelled."));
      });

  return CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
}

}  // namespace client
}  // namespace olp
