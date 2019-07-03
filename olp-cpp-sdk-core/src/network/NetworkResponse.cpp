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

#include "olp/core/network/NetworkResponse.h"

#include <memory>
#include <string>
#include <vector>

namespace olp {
namespace network {

NetworkResponse::NetworkResponse(
    Network::RequestId id, bool cancelled, int status, const std::string& error,
    int maxAge, time_t expires, const std::string& etag,
    const std::string& content_type, std::uint64_t payload_size,
    std::uint64_t payload_offset, std::shared_ptr<std::ostream> payload_stream,
    std::vector<std::pair<std::string, std::string> >&& statistics)
    : error_(error),
      etag_(etag),
      content_type_(content_type),
      payload_stream_(std::move(payload_stream)),
      payload_size_(payload_size),
      payload_offset_(payload_offset),
      statistics_(std::move(statistics)),
      id_(id),
      max_age_(maxAge),
      expires_(expires),
      status_(status),
      cancelled_(cancelled) {}

NetworkResponse::NetworkResponse(Network::RequestId id, int status,
                                 const std::string& error)
    : error_(error), id_(id), status_(status) {}

Network::RequestId NetworkResponse::Id() const { return id_; }

bool NetworkResponse::Cancelled() const { return cancelled_; }

int NetworkResponse::Status() const { return status_; }

const std::string& NetworkResponse::Error() const { return error_; }

int NetworkResponse::MaxAge() const { return max_age_; }

time_t NetworkResponse::Expires() const { return expires_; }

const std::string& NetworkResponse::Etag() const { return etag_; }

const std::string& NetworkResponse::ContentType() const {
  return content_type_;
}

std::uint64_t NetworkResponse::PayloadSize() const { return payload_size_; }

std::uint64_t NetworkResponse::PayloadOffset() const { return payload_offset_; }

std::shared_ptr<std::ostream> NetworkResponse::PayloadStream() const {
  return payload_stream_;
}

const std::vector<std::pair<std::string, std::string> >&
NetworkResponse::Statistics() const {
  return statistics_;
}

}  // namespace network
}  // namespace olp
