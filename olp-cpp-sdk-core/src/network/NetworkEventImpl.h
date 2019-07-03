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

#include <string>
#include <vector>

#include "olp/core/network/NetworkEvent.h"

namespace olp {
namespace network {
/**
 * @brief Default network performance event
 */
class NetworkEventImpl : public NetworkEvent {
 public:
  /**
   * @brief Create a network event based on the network URL.
   * @param url specifies the HTTP/HTTPS URL used in the network request
   */
  NetworkEventImpl(const std::string& url);

  /**
   * @brief Record the event with the network results content length
   * and HTTP status code. Note that the record should be called
   * once the request has completed since this class takes care of
   * timing of the overall response time.
   */
  void Record(
      size_t content_length, int status, int priority, int request_count,
      const std::string& url,
      const std::vector<std::pair<std::string, std::string> >& extra_headers,
      size_t pending) const;

  /**
   * @brief get summary statistics
   */
  static void GetStatistics(size_t& content_lengths, size_t& requests,
                            size_t& errors);

};  // class NetworkEventImpl

}  // namespace network
}  // namespace olp
