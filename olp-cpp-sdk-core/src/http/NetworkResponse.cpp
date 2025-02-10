/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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

#include "olp/core/http/NetworkResponse.h"
#include "olp/core/http/NetworkTypes.h"

namespace olp {
namespace http {

bool NetworkResponse::IsCancelled() const {
  return status_ == static_cast<int>(ErrorCode::CANCELLED_ERROR);
}

int NetworkResponse::GetStatus() const { return status_; }

const std::string& NetworkResponse::GetError() const { return error_; }

NetworkResponse& NetworkResponse::WithStatus(int status) {
  status_ = status;
  return *this;
}

NetworkResponse& NetworkResponse::WithError(std::string error) {
  error_ = std::move(error);
  return *this;
}

RequestId NetworkResponse::GetRequestId() const { return request_id_; }

NetworkResponse& NetworkResponse::WithRequestId(RequestId id) {
  request_id_ = id;
  return *this;
}

uint64_t NetworkResponse::GetBytesUploaded() const { return bytes_uploaded_; }

NetworkResponse& NetworkResponse::WithBytesUploaded(uint64_t bytes_uploaded) {
  bytes_uploaded_ = bytes_uploaded;
  return *this;
}

uint64_t NetworkResponse::GetBytesDownloaded() const {
  return bytes_downloaded_;
}

NetworkResponse& NetworkResponse::WithBytesDownloaded(
    uint64_t bytes_downloaded) {
  bytes_downloaded_ = bytes_downloaded;
  return *this;
}

NetworkResponse& NetworkResponse::WithDiagnostics(Diagnostics diagnostics) {
  diagnostics_ = diagnostics;
  return *this;
}

const boost::optional<Diagnostics>& NetworkResponse::GetDiagnostics() const {
  return diagnostics_;
}

}  // namespace http
}  // namespace olp
