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
#include <ctime>
#include <memory>
#include <string>
#include <vector>

#include "Network.h"

namespace olp {
namespace network {
/**
 * @brief The NetworkResponse class represents network response abstraction for
 * a HTTP request
 */
class CORE_API NetworkResponse {
 public:
  /**
   * @brief NetworkResponse default constructor
   */
  NetworkResponse() = default;

  /**
   * @brief NetworkResponse constructor
   * @param id - network request id
   * @param cancelled - was response cancelled or not
   * @param status - HTTP status code
   * @param error - error string for a request
   * @param maxAge - maximum age from the response
   * @param expires - epoch time when the response is considered stale, -1 for
   * invalid
   * @param etag - ETag of the response
   * @param content_type - Type of the response content
   * @param payload_size - size of response payload
   * @param payload_offset - offset of response payload from begining of the
   * content
   * @param payload_stream - response payload stream
   * @param statistics - vector of statistics
   */
  NetworkResponse(
      Network::RequestId Id, bool Cancelled, int Status,
      const std::string& Error, int MaxAge, time_t Expires,
      const std::string& Etag, const std::string& content_type,
      std::uint64_t payload_size, std::uint64_t payload_offset,
      std::shared_ptr<std::ostream> payload_stream,
      std::vector<std::pair<std::string, std::string> >&& Statistics);

  /**
   * @brief Create network response for simple cases
   * @param id - network request id
   * @param status - HTTP status code
   * @param error - error string for a request
   */
  NetworkResponse(Network::RequestId Id, int Status, const std::string& Error);

  /**
   * @brief request id getter method
   * @return contained request id
   */
  Network::RequestId Id() const;

  /**
   * @brief cancelled getter method
   * @return whether attached request was cancelled
   */
  bool Cancelled() const;

  /**
   * @brief status getter method
   * @return status of HTTP response
   */
  int Status() const;

  /**
   * @brief error string getter method
   * @return error string of failed request
   */
  const std::string& Error() const;

  /**
   * @brief maxAge getter method
   * @return maximum age of HTTP response
   */
  int MaxAge() const;

  /**
   * @brief expire time of reponse
   * @return epoch time when the HTTP response is considered stale, -1 for
   * invalid
   */
  time_t Expires() const;

  /**
   * @brief ETag string getter method
   * @return Etag string of response
   */
  const std::string& Etag() const;

  /**
   * @brief Content type string getter method
   * @return Type of the response content
   */
  const std::string& ContentType() const;

  /**
   * @brief response payload size getter method
   * @return size of response payload
   */
  std::uint64_t PayloadSize() const;

  /**
   * @brief response payload offset getter method
   * @return offset of response payload
   */
  std::uint64_t PayloadOffset() const;

  /**
   * @brief response payload stream getter method
   */
  std::shared_ptr<std::ostream> PayloadStream() const;

  /**
   * @brief get statistics for the request
   * @return vector of statistics
   */
  const std::vector<std::pair<std::string, std::string> >& Statistics() const;

 private:
  std::string error_;
  std::string etag_;
  std::string content_type_;
  std::shared_ptr<std::ostream> payload_stream_;
  std::uint64_t payload_size_{0};
  std::uint64_t payload_offset_{0};
  std::vector<std::pair<std::string, std::string> > statistics_;
  Network::RequestId id_{
      Network::NetworkRequestIdConstants::NetworkRequestIdInvalid};
  int max_age_{0};
  time_t expires_{0};
  int status_{-100};
  bool cancelled_{false};
};

}  // namespace network
}  // namespace olp
