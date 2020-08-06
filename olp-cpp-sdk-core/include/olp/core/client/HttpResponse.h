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

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/CoreApi.h>
#include <olp/core/http/NetworkTypes.h>

namespace olp {
namespace client {

/*
 * @brief Network statistics
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

  /**
   * @brief Overloaded addition operator for accumulating statistics.
   */
  NetworkStatistics& operator+=(const NetworkStatistics& other) {
    bytes_uploaded_ += other.bytes_uploaded_;
    bytes_downloaded_ += other.bytes_downloaded_;
    return *this;
  }

 private:
  uint64_t bytes_uploaded_{0};
  uint64_t bytes_downloaded_{0};
};

/**
 * @brief An HTTP response.
 */
class CORE_API HttpResponse {
 public:
  HttpResponse() = default;
  /**
   * @brief Creates the `HttpResponse` instance.
   *
   * @param status The HTTP status.
   * @param response The response body.
   */
  HttpResponse(int status, std::string response = std::string())  // NOLINT
      : status(status), response(std::move(response)) {}

  /**
   * @brief Creates the `HttpResponse` instance.
   *
   * @param status The HTTP status.
   * @param response The response body.
   */
  HttpResponse(int status, std::stringstream&& response)
      : status(status), response(std::move(response)) {}

  /**
   * @brief Creates the `HttpResponse` instance.
   *
   * @param status The HTTP status.
   * @param response The response body.
   * @param headers Response headers.
   */
  HttpResponse(int status, std::stringstream&& response,
               http::Headers&& headers)
      : status(status),
        response(std::move(response)),
        headers(std::move(headers)) {}

  /**
   * @brief Copy `HttpResponse` content to a vector of unsigned chars.
   *
   * @param output Reference to a vector.
   */
  void GetResponse(std::vector<unsigned char>& output);

  /**
   * @brief Copy `HttpResponse` content to a string.
   *
   * @param output Reference to a string.
   */
  void GetResponse(std::string& output);

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

  /**
   * @brief The HTTP status.
   *
   * @see `ErrorCode` for information on negative status
   * numbers.
   * @see `HttpStatusCode` for information on positive status
   * numbers.
   */
  int status{static_cast<int>(olp::http::ErrorCode::UNKNOWN_ERROR)};
  /**
   * @brief The HTTP response.
   */
  std::stringstream response;

  /**
   * @brief HTTP headers.
   */
  http::Headers headers;

 private:
  NetworkStatistics network_statistics_;
};

inline void HttpResponse::GetResponse(std::vector<unsigned char>& output) {
  response.seekg(0, std::ios::end);
  output.resize(response.tellg());
  response.seekg(0, std::ios::beg);
  response.read(reinterpret_cast<char*>(output.data()), output.size());
}

inline void HttpResponse::GetResponse(std::string& output) {
  output = response.str();
}

}  // namespace client
}  // namespace olp
