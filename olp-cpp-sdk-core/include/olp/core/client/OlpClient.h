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

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CancellationContext.h"
#include "OlpClientSettings.h"

#include <olp/core/CoreApi.h>

namespace olp {
namespace network {
class NetworkRequest;
class NetworkConfig;
}  // namespace network

namespace client {
/**
 * @brief Executes HTTP requests.
 */
class CORE_API OlpClient {
 public:
  OlpClient();
  virtual ~OlpClient() = default;

  /**
   * @brief Sets the base URL used for all requests.

   * @param baseUrl The base URL.
   */
  void SetBaseUrl(const std::string& baseUrl);

  /**
   * @brief Gets the base URL.
   *
   * @return The base URL.
   */
  const std::string& GetBaseUrl() const;

  /**
   * @brief Gets the default headers that are added to each request.
   *
   * @return The default headers.
   */
  std::multimap<std::string, std::string>& GetMutableDefaultHeaders();

  /**
   * @brief Sets the client settings.
   *
   * @param settings The client settings.
   */
  void SetSettings(const OlpClientSettings& settings);

  /**
   * @brief Executes the HTTP request through the network stack.
   *
   * @param path The path that is appended to the base URL.
   * @param method Select one of the following methods: `GET`, `POST`, `DELETE`,
   * or `PUT`.
   * @param query_params The parameters that are appended to the URL path.
   * @param header_params The headers used to customize the request.
   * @param form_params For the `POST` request, populate `form_params` or
   * `post_body`, but not both.
   * @param post_body For the `POST` request, populate `form_params` or
   * `post_body`, but not both. This data must not be modified until
   * the request is completed.
   * @param content_type The content type for the `post_body` or `form_params`.
   * @param callback The function callback used to receive the `HttpResponse`
   * instance.
   *
   * @return The method used to call or to cancel the request.
   */
  CancellationToken CallApi(
      const std::string& path, const std::string& method,
      const std::multimap<std::string, std::string>& query_params,
      const std::multimap<std::string, std::string>& header_params,
      const std::multimap<std::string, std::string>& form_params,
      const std::shared_ptr<std::vector<unsigned char>>& post_body,
      const std::string& content_type,
      const NetworkAsyncCallback& callback) const;

  /**
   * @brief Executes the HTTP request through the network stack in a blocking
   * way.
   *
   * @param path The path that is appended to the base URL.
   * @param method Select one of the following methods: `GET`, `POST`, `DELETE`,
   * or `PUT`.
   * @param query_params The parameters that are appended to the URL path.
   * @param header_params The headers used to customize the request.
   * @param form_params For the `POST` request, populate `form_params` or
   * `post_body`, but not both.
   * @param post_body For the `POST` request, populate `form_params` or
   * `post_body`, but not both. This data must not be modified until
   * the request is completed.
   * @param content_type The content type for the `post_body` or `form_params`.
   * @param context The `CancellationContext` instance that is used to cancel
   * the request.
   *
   * @return The `HttpResponse` instance.
   */
  HttpResponse CallApi(std::string path, std::string method,
                       std::multimap<std::string, std::string> query_params,
                       std::multimap<std::string, std::string> header_params,
                       std::multimap<std::string, std::string> form_params,
                       std::shared_ptr<std::vector<unsigned char>> post_body,
                       std::string content_type,
                       CancellationContext context) const;

 private:
  std::shared_ptr<http::NetworkRequest> CreateRequest(
      const std::string& path, const std::string& method,
      const std::multimap<std::string, std::string>& query_params,
      const std::multimap<std::string, std::string>& header_params,
      const std::shared_ptr<std::vector<unsigned char>>& post_body,
      const std::string& content_type) const;

 private:
  std::string base_url_;
  std::multimap<std::string, std::string> default_headers_;
  OlpClientSettings settings_;
};

}  // namespace client
}  // namespace olp
