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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <olp/core/CoreApi.h>
#include <olp/core/http/NetworkSettings.h>

namespace olp {
namespace http {

/**
 * @brief This class represents network request abstraction for an HTTP request.
 */
class CORE_API NetworkRequest final {
 public:
  /// Short type alias for HTTP request body.
  using RequestBodyType = std::shared_ptr<const std::vector<std::uint8_t>>;
  /// Short type alias for HTTP request header.
  using RequestHeadersType = std::vector<std::pair<std::string, std::string>>;

  /// HTTP method as in https://tools.ietf.org/html/rfc2616.
  enum class HttpVerb {
    GET = 0,    ///< GET Method. RFC2616 section-9.3.
    POST = 1,   ///< POST Method. RFC2616 section-9.5.
    HEAD = 2,   ///< HEAD Method. RFC2616 section-9.4.
    PUT = 3,    ///< PUT Method. RFC2616 section-9.6.
    DEL = 4,    ///< DELETE Method. RFC2616 section-9.7.
    PATCH = 5,  ///< PATCH Method. RFC2068 section-19.6.1.1.
  };

  /**
   * @brief NetworkRequest constructor.
   * @param[in] url HTTP request URL.
   */
  explicit NetworkRequest(std::string url);

  /**
   * @brief Get all HTTP headers.
   * @return vector of HTTP headers.
   */
  const RequestHeadersType& GetHeaders() const;

  /**
   * @brief Add extra HTTP header.
   * @param[in] name Header name.
   * @param[in] value Header value.
   * @return reference to *this.
   */
  NetworkRequest& WithHeader(std::string name, std::string value);

  /**
   * @brief Get request URL.
   * @return request URL.
   */
  const std::string& GetUrl() const;

  /**
   * @brief Set request URL.
   * @param[in] url Request URL.
   * @return reference to *this.
   */
  NetworkRequest& WithUrl(std::string url);

  /**
   * @brief Get HTTP method.
   * @return HTTP method.
   */
  HttpVerb GetVerb() const;

  /**
   * @brief Set HTTP method.
   * @param[in] verb HTTP method.
   * @return reference to *this.
   */
  NetworkRequest& WithVerb(HttpVerb verb);

  /**
   * @brief Get request body.
   * @return shared pointer to vector that contains request body.
   */
  RequestBodyType GetBody() const;

  /**
   * @brief Set request body.
   * @param[in] body Shared pointer to vector that contains request body.
   * @return reference to *this.
   */
  NetworkRequest& WithBody(RequestBodyType body);

  /**
   * @brief Get network settings for this request.
   * @return NetworkSettings object.
   */
  const NetworkSettings& GetSettings() const;

  /**
   * @brief Set network settings for this request.
   * @param[in] settings Instance of NetworkRequest.
   * @return reference to *this.
   */
  NetworkRequest& WithSettings(NetworkSettings settings);

 private:
  /// HTTP request method.
  HttpVerb verb_{HttpVerb::GET};
  /// Request URL.
  std::string url_;
  /// HTTP headers.
  RequestHeadersType headers_;
  /// Body of HTTP request.
  RequestBodyType body_;
  /// Network settings for this request.
  NetworkSettings settings_;
};

}  // namespace http
}  // namespace olp
