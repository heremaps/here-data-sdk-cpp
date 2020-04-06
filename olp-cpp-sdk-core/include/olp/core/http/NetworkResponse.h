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
 * @brief This class represents network response abstraction for an HTTP
 * request.
 */
class CORE_API NetworkResponse final {
 public:
  /**
   * @brief Check if associated request was cancelled.
   *
   * @return true if associated request was cancelled.
   */
  bool IsCancelled() const;

  /**
   * @brief Get HTTP response code.
   *
   * @return HTTP response code.
   */
  int GetStatus() const;

  /**
   * @brief Set HTTP response code.
   *
   * @param[in] status HTTP response code.
   *
   * @return reference to *this.
   */
  NetworkResponse& WithStatus(int status);

  /**
   * @brief Get human-readable error message in case of failed associated
   * request.
   *
   * @return human-readable error message in case of failed associated request.
   */
  const std::string& GetError() const;

  /**
   * @brief Set human-readable error message in case of failed associated
   * request.
   *
   * @param[in] error Human-readable error message in case of failed
   * associated request.
   *
   * @return reference to *this.
   */
  NetworkResponse& WithError(std::string error);

  /**
   * @brief Get id of associated network request.
   *
   * @return id of associated network request.
   */
  RequestId GetRequestId() const;

  /**
   * @brief Set id of associated network request.
   *
   * @param[in] id Id of associated network request.
   *
   * @return reference to *this.
   */
  NetworkResponse& WithRequestId(RequestId id);

  /**
   * @brief Get the number of bytes uploaded during the associated network
   * request.
   *
   * @return The number of bytes uploaded during the associated network request.
   */
  uint64_t GetBytesUploaded() const;

  /**
   * @brief Set the number of bytes uploaded during the associated network
   * request.
   *
   * @param[in] bytes_uploaded Number of uploaded bytes.
   *
   * @return reference to *this.
   */
  NetworkResponse& WithBytesUploaded(uint64_t bytes_uploaded);

  /**
   * @brief Get the number of bytes downloaded during the associated network
   * request.
   *
   * @return The number of bytes downloaded during the associated network
   * request.
   */
  uint64_t GetBytesDownloaded() const;

  /**
   * @brief Set the number of bytes downloaded during the associated network
   * request.
   *
   * @param[in] bytes_downloaded Number of downloaded bytes.
   *
   * @return reference to *this.
   */
  NetworkResponse& WithBytesDownloaded(uint64_t bytes_downloaded);

 private:
  /// Associated request id.
  RequestId request_id_{0};
  /// HTTP response code.
  int status_{0};
  /// Human-readable error message in case of failed associated request.
  std::string error_;
  /// Number of bytes uploaded during network request.
  uint64_t bytes_uploaded_;
  /// Number of bytes downloaded during network request.
  uint64_t bytes_downloaded_;
};

}  // namespace http
}  // namespace olp
