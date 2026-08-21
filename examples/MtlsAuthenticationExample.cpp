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

#include "MtlsAuthenticationExample.h"

#include <olp/authentication/MtlsTokenProvider.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/Network.h>
#include <olp/core/logging/Log.h>
#include <olp/core/utils/Url.h>

#include <fstream>
#include <future>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace {
constexpr auto kLogTag = "mtls-authentication-example";

constexpr auto kDiscoverBaseUrl = "https://discover.search.hereapi.com";
constexpr auto kDiscoverPath = "/v1/discover";

// Only the beginning of the response body is logged, the full document can
// be large.
constexpr size_t kMaxLoggedResponseSize = 512;

std::string ReadFile(const std::string& path) {
  std::ifstream stream(path, std::ios::in | std::ios::binary);
  if (!stream) {
    return {};
  }
  std::ostringstream contents;
  contents << stream.rdbuf();
  return contents.str();
}

// Presents the client certificate during the mTLS handshake to retrieve a
// bearer token. Returns an empty string in case of a failure.
std::string RequestAccessToken(const std::string& cert_path,
                               const std::string& key_path,
                               const std::string& ca_path,
                               const std::string& scope) {
  const auto client_cert_pem = ReadFile(cert_path);
  const auto client_key_pem = ReadFile(key_path);

  if (client_cert_pem.empty() || client_key_pem.empty()) {
    OLP_SDK_LOG_ERROR_F(kLogTag,
                        "Failed to read certificate or key file, cert='%s', "
                        "key='%s'",
                        cert_path.c_str(), key_path.c_str());
    return {};
  }

  olp::authentication::MtlsSettings settings;
  settings.mtls_properties.client_cert_pem = client_cert_pem;
  settings.mtls_properties.client_key_pem = client_key_pem;
  if (!ca_path.empty()) {
    settings.mtls_properties.ca_cert_pem = ReadFile(ca_path);
  }
  if (!scope.empty()) {
    settings.mtls_properties.scope = scope;
  }

  const olp::authentication::MtlsTokenProviderDefault token_provider(
      std::move(settings));

  olp::client::CancellationContext context;
  const auto token_response = token_provider(context);

  if (!token_response.IsSuccessful()) {
    OLP_SDK_LOG_ERROR_F(
        kLogTag, "mTLS sign in - Failure(%d): %s",
        static_cast<int>(token_response.GetError().GetErrorCode()),
        token_response.GetError().GetMessage().c_str());
    return {};
  }

  OLP_SDK_LOG_INFO_F(kLogTag, "mTLS sign in - Success, expires in %lld s",
                     static_cast<long long>(
                         token_response.GetResult().GetExpiresIn().count()));

  return token_response.GetResult().GetAccessToken();
}

// Calls the HERE Discover Search API with the access token passed as the
// bearer token.
bool CallDiscoverApi(const std::string& access_token) {
  std::shared_ptr<olp::http::Network> http_client = olp::client::
      OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();

  const std::multimap<std::string, std::string> query_params = {
      {"q", "döner"},
      {"at", "52.53083376480065,13.38469608732926"},
      {"limit", "3"}};

  const auto url =
      olp::utils::Url::Construct(kDiscoverBaseUrl, kDiscoverPath, query_params);

  auto request = olp::http::NetworkRequest(url)
                     .WithVerb(olp::http::NetworkRequest::HttpVerb::GET)
                     .WithHeader("Authorization", "Bearer " + access_token);

  auto payload = std::make_shared<std::stringstream>();

  std::promise<olp::http::NetworkResponse> promise;
  auto future = promise.get_future();

  const auto outcome =
      http_client->Send(std::move(request), payload,
                        [&promise](olp::http::NetworkResponse response) {
                          promise.set_value(std::move(response));
                        });

  if (!outcome.IsSuccessful()) {
    OLP_SDK_LOG_ERROR_F(
        kLogTag, "Discover request was not sent - Failure: %s",
        olp::http::ErrorCodeToString(outcome.GetErrorCode()).c_str());
    return false;
  }

  const auto response = future.get();

  if (response.GetStatus() != olp::http::HttpStatusCode::OK) {
    OLP_SDK_LOG_ERROR_F(kLogTag, "Discover request - Failure(%d): %s",
                        response.GetStatus(), response.GetError().c_str());
    return false;
  }

  auto body = payload->str();
  OLP_SDK_LOG_INFO_F(kLogTag, "Discover request - Success, response: %s%s",
                     body.substr(0, kMaxLoggedResponseSize).c_str(),
                     body.size() > kMaxLoggedResponseSize ? "..." : "");

  return true;
}
}  // namespace

int RunExampleMtlsAuthentication(const std::string& cert_path,
                                 const std::string& key_path,
                                 const std::string& ca_path,
                                 const std::string& scope) {
  const auto access_token =
      RequestAccessToken(cert_path, key_path, ca_path, scope);
  if (access_token.empty()) {
    return -1;
  }

  return CallDiscoverApi(access_token) ? 0 : -1;
}
