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
 * @brief A network request abstraction for an HTTP request.
 */
class CORE_API NetworkRequest final {
 public:
  /// The short type alias for the HTTP request body.
  using RequestBodyType = std::shared_ptr<const std::vector<std::uint8_t>>;

  /// The HTTP method, as specified at https://tools.ietf.org/html/rfc2616.
  enum class HttpVerb {
    GET = 0,     ///< The GET method (RFC2616, section-9.3).
    POST = 1,    ///< The POST method (RFC2616, section-9.5).
    HEAD = 2,    ///< The HEAD method (RFC2616 section-9.4).
    PUT = 3,     ///< The PUT method (RFC2616 section-9.6).
    DEL = 4,     ///< The DELETE method (RFC2616 section-9.7).
    PATCH = 5,   ///< The PATCH method (RFC2068 section-19.6.1.1).
    OPTIONS = 6, ///< The OPTIONS method (RFC2616 section-9.2).
  };

  /**
   * @brief Creates the `NetworkRequest` instance.
   *
   * @param[in] url The URL of the HTTP request.
   */
  explicit NetworkRequest(std::string url);

  /**
   * @brief Gets all HTTP headers.
   *
   * @return The vector of the HTTP headers.
   */
  const Headers& GetHeaders() const;

  /**
   * @brief Gets the mutable reference to the HTTP headers.
   *
   * @return The mutable reference to vector of the HTTP headers.
   */
  Headers& GetMutableHeaders();

  /**
   * @brief Adds an extra HTTP header.
   *
   * @param[in] name The header name.
   * @param[in] value The header value.
   *
   * @return A reference to *this.
   */
  NetworkRequest& WithHeader(std::string name, std::string value);

  /**
   * @brief Gets the request URL.
   *
   * @return The request URL.
   */
  const std::string& GetUrl() const;

  /**
   * @brief Sets the request URL.
   *
   * @param[in] url The request URL.
   *
   * @return A reference to *this.
   */
  NetworkRequest& WithUrl(std::string url);

  /**
   * @brief Gets the HTTP method.
   *
   * @return The HTTP method.
   */
  HttpVerb GetVerb() const;

  /**
   * @brief Sets the HTTP method.
   *
   * @param[in] verb The HTTP method.
   *
   * @return A reference to *this.
   */
  NetworkRequest& WithVerb(HttpVerb verb);

  /**
   * @brief Gets the request body.
   *
   * @return The shared pointer to the vector that contains the request body.
   */
  RequestBodyType GetBody() const;

  /**
   * @brief Sets the request body.
   *
   * @param[in] body The shared pointer to the vector that contains the request
   * body.
   *
   * @return A reference to *this.
   */
  NetworkRequest& WithBody(RequestBodyType body);

  /**
   * @brief Gets the network settings for this request.
   *
   * @return The `NetworkSettings` object.
   */
  const NetworkSettings& GetSettings() const;

  /**
   * @brief Sets network settings for this request.
   *
   * @param[in] settings The `NetworkRequest` instance.
   *
   * @return A reference to *this.
   */
  NetworkRequest& WithSettings(NetworkSettings settings);

 private:
  /// The HTTP request method.
  HttpVerb verb_{HttpVerb::GET};
  /// The request URL.
  std::string url_;
  /// The HTTP headers.
  Headers headers_;
  /// The body of the HTTP request.
  RequestBodyType body_;
  /// The network settings for this request.
  NetworkSettings settings_{};
};

}  // namespace http
}  // namespace olp
