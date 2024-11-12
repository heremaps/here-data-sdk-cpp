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
      : status_(status), response_(std::move(response)) {}

  /**
   * @brief Creates the `HttpResponse` instance.
   *
   * @param status The HTTP status.
   * @param response The response body.
   *
   */
  HttpResponse(int status, std::stringstream&& response)
      : status_(status), response_(std::move(response)) {}

  /**
   * @brief Creates the `HttpResponse` instance.
   *
   * @param status The HTTP status.
   * @param response The response body.
   * @param headers Response headers.
   *
   */
  HttpResponse(int status, std::stringstream&& response, http::Headers headers)
      : status_(status),
        response_(std::move(response)),
        headers_(std::move(headers)) {}

  /**
   * @brief A copy constructor.
   *
   * This copy constructor creates a deep copy of the response body if a
   * non-shareable container type is used.
   *
   * @param other The instance of `HttpStatus` to copy from.
   */
  HttpResponse(const HttpResponse& other)
      : status_(other.status_),
        headers_(other.headers_),
        network_statistics_(other.network_statistics_) {
    response_ << other.response_.rdbuf();
    if (!response_.good()) {
      // Depending on the users handling of the stringstream it might be that
      // the read position is already at the end and thus operator<< cannot
      // read anything, so lets try with the safer but more memory intensive
      // solution as a second step.
      response_.str(other.GetResponseAsString());
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
      status_ = other.status_;
      response_ = std::stringstream{};
      response_ << other.response_.rdbuf();
      headers_ = other.headers_;
      network_statistics_ = other.network_statistics_;
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
    response_.seekg(0, std::ios::end);
    const auto pos = response_.tellg();
    if (pos > 0) {
      output.resize(pos);
    }
    response_.seekg(0, std::ios::beg);
    response_.read(reinterpret_cast<char*>(output.data()), output.size());
    response_.seekg(0, std::ios::beg);
  }

  /**
   * @brief Copy `HttpResponse` content to a string.
   *
   * @param output Reference to a string.
   */
  void GetResponse(std::string& output) const { output = response_.str(); }

  /**
   * @brief Get the response body as a vector of unsigned chars.
   *
   * @return The response body as a vector of unsigned chars.
   */
  std::vector<unsigned char> GetResponseAsBytes() {
    std::vector<unsigned char> bytes;
    GetResponse(bytes);
    return bytes;
  }

  /**
   * @brief Renders `HttpResponse` content to a string.
   *
   * @return String representation of the response.
   */
  std::string GetResponseAsString() const {
    std::string result;
    GetResponse(result);
    return result;
  }

  /**
   * @brief Return the reference to the response object.
   *
   * @return The reference to the response object.
   */
  std::stringstream& GetRawResponse() { return response_; }

  /**
   * @brief Return the const reference to the response headers.
   *
   * @return The const reference to the headers vector.
   */
  const http::Headers& GetHeaders() const { return headers_; }

  /**
   * @brief Return the response status.
   *
   * The response status can either be an `ErrorCode` if negative or a
   * `HttpStatusCode` if positive.
   *
   * @return The response status.
   */
  int GetStatus() const { return status_; }

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

 private:
  int status_{static_cast<int>(olp::http::ErrorCode::UNKNOWN_ERROR)};
  std::stringstream response_;
  http::Headers headers_;
  NetworkStatistics network_statistics_;
};

}  // namespace client
}  // namespace olp
