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

#include <olp/core/network/HttpResponse.h>
#include "NetworkAsyncHandlerImpl.h"
#include "olp/core/http/NetworkConstants.h"
#include "olp/core/network/Network.h"
#include "olp/core/network/NetworkResponse.h"

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
}  // namespace

CancellationToken DefaultNetworkAsyncHandler(
    const network::NetworkRequest& request,
    const network::NetworkConfig& config,
    const NetworkAsyncCallback& callback) {
  static NetworkAsyncHandlerImpl handler;

  return handler(request, config, callback);
}

OlpClient::OlpClient() : network_async_handler_(DefaultNetworkAsyncHandler) {}

void OlpClient::SetBaseUrl(const std::string& baseUrl) { base_url_ = baseUrl; }

const std::string& OlpClient::GetBaseUrl() const { return base_url_; }

std::multimap<std::string, std::string>& OlpClient::GetMutableDefaultHeaders() {
  return default_headers_;
}

void OlpClient::SetSettings(const OlpClientSettings& settings) {
  settings_ = settings;

  if (settings.network_async_handler) {
    network_async_handler_ = settings.network_async_handler.get();
  }
}

std::shared_ptr<network::NetworkRequest> OlpClient::CreateRequest(
    const std::string& path, const std::string& method,
    const std::multimap<std::string, std::string>& queryParams,
    const std::multimap<std::string, std::string>& headerParams,
    const std::shared_ptr<std::vector<unsigned char>>& postBody,
    const std::string& contentType) const {
  network::NetworkRequest::HttpVerb httpVerb =
      network::NetworkRequest::HttpVerb::GET;
  auto reqMethod = method;
  if (reqMethod.compare("GET") == 0)
    httpVerb = network::NetworkRequest::HttpVerb::GET;
  else if (reqMethod.compare("PUT") == 0)
    httpVerb = network::NetworkRequest::HttpVerb::PUT;
  else if (reqMethod.compare("POST") == 0)
    httpVerb = network::NetworkRequest::HttpVerb::POST;
  else if (reqMethod.compare("DELETE") == 0)
    httpVerb = network::NetworkRequest::HttpVerb::DEL;

  std::string uri = base_url_ + path;
  if (!queryParams.empty()) {
    std::string queryParamsString = "?";
    auto first = true;
    for (auto queryParam : queryParams) {
      if (!first) {
        queryParamsString.append("&");
      }
      first = false;
      queryParamsString.append(queryParam.first)
          .append("=")
          .append(queryParam.second);
    }
    uri.append(queryParamsString);
  }
  auto networkRequest = std::make_shared<network::NetworkRequest>(
      uri, 0, network::NetworkRequest::PriorityDefault, httpVerb);

  if (settings_.authentication_settings) {
    std::string bearer = http::kBearer + std::string(" ") +
                         settings_.authentication_settings.get().provider();
    networkRequest->AddHeader(http::kAuthorizationHeader, bearer);
  }

  for (const auto& header : default_headers_) {
    networkRequest->AddHeader(header.first, header.second);
  }

  std::string custom_user_agent;
  for (const auto& header : headerParams) {
    // Merge all User-Agent headers into one header.
    // This is required for (at least) iOS network implementation,
    // which uses headers dictionary without any duplicates.
    // User agents entries are usually separated by a whitespace, e.g.
    // Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:47.0) Firefox/47.0
    if (CaseInsensitiveCompare(header.first, http::kUserAgentHeader)) {
      custom_user_agent += header.second + std::string(" ");
    } else {
      networkRequest->AddHeader(header.first, header.second);
    }
  }

  custom_user_agent += http::kOlpSdkUserAgent;
  networkRequest->AddHeader(http::kUserAgentHeader, custom_user_agent);

  if (!contentType.empty()) {
    networkRequest->AddHeader(http::kContentTypeHeader, contentType);
  }

  networkRequest->SetContent(postBody);
  return networkRequest;
}

NetworkAsyncCallback GetRetryCallback(
    int currentTry, int currentBackdownPeriod, const RetrySettings& settings,
    const NetworkAsyncCallback& callback,
    const std::shared_ptr<network::NetworkRequest>& networkRequest,
    const NetworkAsyncHandler& networkAsyncHandler,
    const network::NetworkConfig& config,
    const std::weak_ptr<CancellationContext>& weak_cancel_context) {
  ++currentTry;
  return [=](network::HttpResponse response) {
    if (currentTry >= settings.max_attempts ||
        !settings.retry_condition(response)) {
      callback(response);
    } else {
      // TODO there must be a better way
      std::this_thread::sleep_for(
          std::chrono::milliseconds(currentBackdownPeriod));

      auto nextBackdownPeriod = settings.backdown_policy(currentBackdownPeriod);
      auto cancel_context = weak_cancel_context.lock();
      if (cancel_context) {
        cancel_context->ExecuteOrCancelled(
            [&]() -> CancellationToken {
              return networkAsyncHandler(
                  *networkRequest, config,
                  GetRetryCallback(currentTry, nextBackdownPeriod, settings,
                                   callback, networkRequest,
                                   networkAsyncHandler, config,
                                   weak_cancel_context));
            },
            [callback]() {
              callback(
                  network::HttpResponse(network::Network::ErrorCode::Cancelled,
                                        "Operation Cancelled."));
            });
      } else {
        callback(network::HttpResponse(
            network::Network::ErrorCode::UnknownError, "Unknown error."));
      }
    }
  };
}

CancellationToken OlpClient::CallApi(
    const std::string& path, const std::string& method,
    const std::multimap<std::string, std::string>& queryParams,
    const std::multimap<std::string, std::string>& headerParams,
    const std::multimap<std::string, std::string>& /*formParams*/,
    const std::shared_ptr<std::vector<unsigned char>>& postBody,
    const std::string& contentType,
    const NetworkAsyncCallback& callback) const {
  auto networkRequest = CreateRequest(path, method, queryParams, headerParams,
                                      postBody, contentType);
  auto cancel_context = std::make_shared<CancellationContext>();

  network::NetworkConfig config;
  config.SetTimeouts(settings_.retry_settings.timeout,
                     config.TransferTimeout());
  if (settings_.proxy_settings) {
    config.SetProxy(*settings_.proxy_settings);
  }

  RetrySettings retrySettings;
  retrySettings.max_attempts = settings_.retry_settings.max_attempts;
  retrySettings.timeout = settings_.retry_settings.timeout;
  retrySettings.initial_backdown_period =
      settings_.retry_settings.initial_backdown_period;
  retrySettings.backdown_policy = settings_.retry_settings.backdown_policy;
  retrySettings.retry_condition = settings_.retry_settings.retry_condition;

  NetworkAsyncCallback retryCallback = GetRetryCallback(
      0, settings_.retry_settings.initial_backdown_period, retrySettings,
      callback, networkRequest, network_async_handler_, config, cancel_context);

  cancel_context->ExecuteOrCancelled(
      [=]() -> CancellationToken {
        return network_async_handler_(*networkRequest, config, retryCallback);
      },
      [callback]() {
        callback(network::HttpResponse(network::Network::ErrorCode::Cancelled,
                                       "Operation Cancelled."));
      });

  return CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
}

}  // namespace client
}  // namespace olp
