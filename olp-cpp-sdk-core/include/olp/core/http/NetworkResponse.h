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

#include <string>

#include <olp/core/CoreApi.h>
#include <olp/core/http/NetworkTypes.h>

namespace olp {
namespace http {
/**
 * @brief A network response abstraction for the HTTP request.
 */
class CORE_API NetworkResponse final {
 public:
  /**
   * @brief Checks if the associated request was canceled.
   *
   * @return True if the associated request was canceled; false otherwise.
   */
  bool IsCancelled() const;

  /**
   * @brief Gets the HTTP response code.
   *
   * @return The HTTP response code.
   */
  int GetStatus() const;

  /**
   * @brief Sets the HTTP response code.
   *
   * @param[in] status The HTTP response code.
   *
   * @return A reference to *this.
   */
  NetworkResponse& WithStatus(int status);

  /**
   * @brief Gets the human-readable error message if the associated
   * request failed.
   *
   * @return The human-readable error message if the associated request failed.
   */
  const std::string& GetError() const;

  /**
   * @brief Sets the human-readable error message if the associated
   * request failed.
   *
   * @param[in] error The human-readable error message if the associated request
   * failed.
   *
   * @return A reference to *this.
   */
  NetworkResponse& WithError(std::string error);

  /**
   * @brief Gets the ID of the associated network request.
   *
   * @return The ID of the associated network request.
   */
  RequestId GetRequestId() const;

  /**
   * @brief Sets the ID of the associated network request.
   *
   * @param[in] id The ID of the associated network request.
   *
   * @return A reference to *this.
   */
  NetworkResponse& WithRequestId(RequestId id);

  /**
   * @brief Gets the number of bytes uploaded during the associated network
   * request.
   *
   * @return The number of bytes uploaded during the associated network request.
   */
  uint64_t GetBytesUploaded() const;

  /**
   * @brief Sets the number of bytes uploaded during the associated network
   * request.
   *
   * @param[in] bytes_uploaded The number of uploaded bytes.
   *
   * @return A reference to *this.
   */
  NetworkResponse& WithBytesUploaded(uint64_t bytes_uploaded);

  /**
   * @brief Gets the number of bytes downloaded during the associated network
   * request.
   *
   * @return The number of bytes downloaded during the associated network
   * request.
   */
  uint64_t GetBytesDownloaded() const;

  /**
   * @brief Sets the number of bytes downloaded during the associated network
   * request.
   *
   * @param[in] bytes_downloaded The number of downloaded bytes.
   *
   * @return A reference to *this.
   */
  NetworkResponse& WithBytesDownloaded(uint64_t bytes_downloaded);

 private:
  /// The associated request ID.
  RequestId request_id_{0};
  /// The HTTP response code.
  int status_{0};
  /// The human-readable error message if the associated request failed.
  std::string error_;
  /// The number of bytes uploaded during the network request.
  uint64_t bytes_uploaded_;
  /// The number of bytes downloaded during the network request.
  uint64_t bytes_downloaded_;
};

}  // namespace http
}  // namespace olp
