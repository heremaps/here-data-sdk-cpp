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
 * @brief Async network handler that delegates to the core network component.
 */
CancellationToken DefaultNetworkAsyncHandler(
    const network::NetworkRequest& request,
    const network::NetworkConfig& config, const NetworkAsyncCallback& callback);

/**
 * @brief This class is responsible for executing the REST requests.
 */
class CORE_API OlpClient {
 public:
  OlpClient();
  virtual ~OlpClient() = default;

  /**
   * @brief Set the base url used for all requests.
   * @param baseUrl Base url.
   */
  void SetBaseUrl(const std::string& baseUrl);

  /**
   * @brief Get the base url.
   */
  const std::string& GetBaseUrl() const;

  /**
   * @brief Get the default headers that is added for each request.
   */
  std::multimap<std::string, std::string>& GetMutableDefaultHeaders();

  /**
   * @brief Set the client settings.
   * @param settings Client settings.
   */
  void SetSettings(const OlpClientSettings& settings);

  /**
   * @brief Execute the REST request through the network stack
   * @param path Path that is appended to the base url.
   * @param method Choice of GET, POST, DELETE, PUT.
   * @param query_params Params that is appended to the path url.
   * @param header_params Headers to customize request.
   * @param form_params For a POST request, form_params or post_body should be
   * populated, but not both.
   * @param post_body For a POST request, form_params or post_body should be
   * populated, but not both.
   * This data must not be modified until the request is completed.
   * @param content_type Content type for the post_body or form_params.
   * @param callback a function callback to receive the HttpResponse.
   *
   * @return A method to call to cancel the request.
   */
  CancellationToken CallApi(
      const std::string& path, const std::string& method,
      const std::multimap<std::string, std::string>& query_params,
      const std::multimap<std::string, std::string>& header_params,
      const std::multimap<std::string, std::string>& form_params,
      const std::shared_ptr<std::vector<unsigned char>>& post_body,
      const std::string& content_type,
      const NetworkAsyncCallback& callback) const;

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
  NetworkAsyncHandler network_async_handler_;
  OlpClientSettings settings_;
};

}  // namespace client
}  // namespace olp
