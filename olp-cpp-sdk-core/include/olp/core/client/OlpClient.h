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

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <olp/core/CoreApi.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/OlpClientSettings.h>

namespace olp {
namespace client {
/**
 * @brief Executes HTTP requests by using the base url and the provided
 * parameters and body. This class will handle retries based on the
 * RetrySettings and will merge all similar URL requests into one.
 */
class CORE_API OlpClient {
 public:
  /// An alias for the parameters and headers type.
  using ParametersType = std::multimap<std::string, std::string>;

  /// An alias for the HTTP request body.
  using RequestBodyType = std::shared_ptr<std::vector<std::uint8_t>>;

  OlpClient();

  /**
   * @brief Creates the `OlpClient` instance.
   *
   * @param settings The `OlpClientSettings` instance.
   * @param base_url The base URL to be used for all outgoing requests.
   */
  OlpClient(const OlpClientSettings& settings, std::string base_url);
  virtual ~OlpClient();

  /// A copy constructor.
  OlpClient(const OlpClient&);

  /// An assignement operator.
  OlpClient& operator=(const OlpClient&);

  /// A move constructor.
  OlpClient(OlpClient&&) noexcept;

  /// A move assignement operator.
  OlpClient& operator=(OlpClient&&) noexcept;

  /**
   * @brief Sets the base URL used for all requests.
   *
   * @note The base URL can change over time and it is thread safe to change it.
   *
   * @param base_url The new base URL to be used for all outgoing requests.
   */
  void SetBaseUrl(const std::string& base_url);

  /**
   * @brief Gets the base URL.
   *
   * @return The base URL.
   */
  std::string GetBaseUrl() const;

  /**
   * @brief Gets the default headers that are added to each request.
   *
   * @note Do not change this while requests are ongoing.
   *
   * @return The default headers.
   */
  ParametersType& GetMutableDefaultHeaders();

  /**
   * @brief Getter function to retrieve client settings.
   *
   * @return The client settings
   */
  const OlpClientSettings& GetSettings() const;

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
  CancellationToken CallApi(const std::string& path, const std::string& method,
                            const ParametersType& query_params,
                            const ParametersType& header_params,
                            const ParametersType& form_params,
                            const RequestBodyType& post_body,
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
                       ParametersType query_params,
                       ParametersType header_params, ParametersType form_params,
                       RequestBodyType post_body, std::string content_type,
                       CancellationContext context) const;

  /**
   * @brief Executes the HTTP request through the network stack in a blocking
   * way. The response content is consumed via data callback.
   *
   * @param path The path that is appended to the base URL.
   * @param method Select one of the following methods: `GET`, `POST`, `DELETE`,
   * or `PUT`.
   * @param query_params The parameters that are appended to the URL path.
   * @param header_params The headers used to customize the request.
   * @param data_callback The network data callback to retrieve content.
   * @param post_body For the `POST` request, populate `form_params` or
   * `post_body`, but not both. This data must not be modified until
   * the request is completed.
   * @param content_type The content type for the `post_body` or `form_params`.
   * @param context The `CancellationContext` instance that is used to cancel
   * the request.
   *
   * @return The `HttpResponse` instance.
   */
  HttpResponse CallApiStream(std::string path, std::string method,
                             ParametersType query_params,
                             ParametersType header_params,
                             http::Network::DataCallback data_callback,
                             RequestBodyType post_body,
                             std::string content_type,
                             CancellationContext context) const;

 private:
  class OlpClientImpl;
  std::shared_ptr<OlpClientImpl> impl_;
};

}  // namespace client
}  // namespace olp
