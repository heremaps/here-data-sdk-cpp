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

#include "olp/core/http/NetworkResponse.h"

namespace olp {
namespace http {

bool NetworkResponse::IsCancelled() const { return cancelled_; }

int NetworkResponse::GetStatus() const { return status_; }

const std::string& NetworkResponse::GetError() const { return error_; }

NetworkResponse& NetworkResponse::WithCancelled(bool cancelled) {
  cancelled_ = cancelled;
  return *this;
}

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

}  // namespace http
}  // namespace olp
