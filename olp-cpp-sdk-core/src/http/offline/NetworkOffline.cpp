/*
 * Copyright (C) 2024 HERE Europe B.V.
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

#include "NetworkOffline.h"

#include <memory>

#include "olp/core/http/HttpStatusCode.h"
#include "olp/core/logging/Log.h"

namespace olp {
namespace http {

namespace {

const char* kLogTag = "NetworkOffline";

}  // anonymous namespace

NetworkOffline::NetworkOffline() {
  OLP_SDK_LOG_TRACE(kLogTag, "Created NetworkOffline with address=" << this);
}

NetworkOffline::~NetworkOffline() {
  OLP_SDK_LOG_TRACE(kLogTag, "Destroyed NetworkOffline object, this=" << this);
}

SendOutcome NetworkOffline::Send(NetworkRequest request,
                                 std::shared_ptr<std::ostream> payload,
                                 Network::Callback callback,
                                 Network::HeaderCallback header_callback,
                                 Network::DataCallback data_callback) {
  OLP_SDK_CORE_UNUSED(payload);
  OLP_SDK_CORE_UNUSED(callback);
  OLP_SDK_CORE_UNUSED(header_callback);
  OLP_SDK_CORE_UNUSED(data_callback);
  OLP_SDK_LOG_ERROR(
      kLogTag, "Send failed - network is offline, url=" << request.GetUrl());
  return SendOutcome(ErrorCode::OFFLINE_ERROR);
}

void NetworkOffline::Cancel(RequestId id) {
  OLP_SDK_LOG_ERROR(kLogTag, "Cancel failed - network is offline, id=" << id);
  return;
}

}  // namespace http
}  // namespace olp
