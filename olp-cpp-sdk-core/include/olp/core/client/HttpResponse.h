/*
 * Copyright (C) 2019-2022 HERE Europe B.V.
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

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/CoreApi.h>
#include <olp/core/http/NetworkTypes.h>

namespace olp {
namespace client {

/**
 * @brief Network statistics with information on the outbound and inbound trafic
 * during API calls.
 */
class NetworkStatistics {
 public:
  NetworkStatistics() = default;

  /**
   * @brief Creates the `NetworkStatistics` instance.
   *
   * @param bytes_uploaded The number of bytes of outbound traffic during API
   * call.
   * @param bytes_downloaded The number of bytes of inbound traffic during API
   * call.
   */
  NetworkStatistics(uint64_t bytes_uploaded, uint64_t bytes_downloaded)
      : bytes_uploaded_{bytes_uploaded}, bytes_downloaded_{bytes_downloaded} {}

  /**
   * @brief Get the number of bytes of outbound traffic.
   *
   * @return Number of bytes.
   */
  uint64_t GetBytesUploaded() const { return bytes_uploaded_; }

  /**
   * @brief Get the number of bytes of inbound traffic.
   *
   * @return Number of bytes.
   */
  uint64_t GetBytesDownloaded() const { return bytes_downloaded_; }

  /// An overloaded addition operator for accumulating statistics.
  NetworkStatistics& operator+=(const NetworkStatistics& other) {
    bytes_uploaded_ += other.bytes_uploaded_;
    bytes_downloaded_ += other.bytes_downloaded_;
    return *this;
  }

  /// An overloaded addition operator for accumulating statistics.
  NetworkStatistics operator+(const NetworkStatistics& other) const {
    NetworkStatistics statistics(*this);
    statistics += other;
    return statistics;
  }

 private:
  uint64_t bytes_uploaded_{0};
  uint64_t bytes_downloaded_{0};
};

/**
 * @brief This class represents the HTTP response created from the
 * NetworkResponse and the request body.
 */
class CORE_API HttpResponse {
 public:
  HttpResponse() = default;
  virtual ~HttpResponse() = default;

  /**
   * @brief Creates the `HttpResponse` instance.
   *
   * @param status The HTTP status.
   * @param response The response body.
   */
  HttpResponse(int status, std::string response = {})  // NOLINT
      : status(status), response(std::move(response)) {}

  /**
   * @brief Creates the `HttpResponse` instance.
   *
   * @param status The HTTP status.
   * @param response The response body.
   *
   */
  HttpResponse(int status, std::stringstream&& response)
      : status(status), response(std::move(response)) {}

  /**
   * @brief Creates the `HttpResponse` instance.
   *
   * @param status The HTTP status.
   * @param response The response body.
   * @param headers Response headers.
   *
   */
  HttpResponse(int status, std::stringstream&& response, http::Headers headers)
      : status(status),
        response(std::move(response)),
        headers(std::move(headers)) {}

  /**
   * @brief A copy constructor.
   *
   * This copy constructor creates a deep copy of the response body if a
   * non-shareable container type is used.
   *
   * @param other The instance of `HttpStatus` to copy from.
   */
  HttpResponse(const HttpResponse& other)
      : status(other.status), headers(other.headers) {
    response << other.response.rdbuf();
    if (!response.good()) {
      // Depending on the users handling of the stringstream it might be that
      // the read position is already at the end and thus operator<< cannot
      // read anything, so lets try with the safer but more memory intensive
      // solution as a second step.
      response.str(other.response.str());
    }
  }

  /**
   * @brief A copy assignment operator.
   *
   * This copy assignment operator creates a deep copy of the response body if a
   * non-shareable container type is used.
   *
   * @param other The instance of `HttpStatus` to copy from.
   */
  HttpResponse& operator=(const HttpResponse& other) {
    if (this != &other) {
      status = other.status;
      response = std::stringstream{};
      response << other.response.rdbuf();
      headers = other.headers;
    }

    return *this;
  }

  /// A default move constructor.
  HttpResponse(HttpResponse&&) = default;

  /// A default move assignment operator.
  HttpResponse& operator=(HttpResponse&&) = default;

  /**
   * @brief Copy `HttpResponse` content to a vector of unsigned chars.
   *
   * @param output Reference to a vector.
   */
  void GetResponse(std::vector<unsigned char>& output) {
    response.seekg(0, std::ios::end);
    const auto pos = response.tellg();
    if (pos > 0) {
      output.resize(pos);
    }
    response.seekg(0, std::ios::beg);
    response.read(reinterpret_cast<char*>(output.data()), output.size());
    response.seekg(0, std::ios::beg);
  }

  /**
   * @brief Copy `HttpResponse` content to a string.
   *
   * @param output Reference to a string.
   */
  void GetResponse(std::string& output) const { output = response.str(); }

  /**
   * @brief Return the const reference to the response headers.
   *
   * @return The const reference to the headers vector.
   */
  const http::Headers& GetHeaders() const { return headers; }

  /**
   * @brief Return the response status.
   *
   * The response status can either be an `ErrorCode` if negative or a
   * `HttpStatusCode` if positive.
   *
   * @return The response status.
   */
  int GetStatus() const { return status; }

  /**
   * @brief Set `NetworkStatistics`.
   *
   * @param network_statistics Instance of `NetworkStatistics`.
   */
  void SetNetworkStatistics(NetworkStatistics network_statistics) {
    network_statistics_ = network_statistics;
  }

  /**
   * @brief Get the `NetworkStatistics`.
   *
   * @return Instance of `NetworkStatistics` previously set.
   */
  const NetworkStatistics& GetNetworkStatistics() const {
    return network_statistics_;
  }

  /// The HTTP Status. This can be either a `ErrorCode` if negative or a
  /// `HttpStatusCode` if positive.
  /// @deprecated: This field will be marked as private by 01.2021.
  /// Please do not use directly, use `GetStatus()` method instead.
  int status{static_cast<int>(olp::http::ErrorCode::UNKNOWN_ERROR)};
  /// The HTTP response.
  /// @deprecated: This field will be marked as private by 01.2021.
  /// Please do not use directly, use `GetResponse()` methods instead.
  std::stringstream response;
  /// HTTP headers.
  /// @deprecated: This field will be marked as private by 01.2021.
  /// Please do not use directly, use `GetHeaders()` method instead.
  http::Headers headers;

 private:
  NetworkStatistics network_statistics_;
};

}  // namespace client
}  // namespace olp
