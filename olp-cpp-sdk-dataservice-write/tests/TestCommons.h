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

#include <olp/core/client/OlpClient.h>
#include <olp/core/http/Network.h>

#include <functional>
#include <future>
#include <memory>
#include <tuple>

using NetworkCallback = std::function<olp::http::SendOutcome(
    olp::http::NetworkRequest, olp::http::Network::Payload,
    olp::http::Network::Callback, olp::http::Network::HeaderCallback,
    olp::http::Network::DataCallback)>;

using CancelCallback = std::function<void(olp::http::RequestId)>;

struct MockedResponseInformation {
  int status;
  const char* data;
};

std::tuple<olp::http::RequestId, NetworkCallback, CancelCallback>
generateNetworkMocks(std::shared_ptr<std::promise<void>> pre_signal,
                     std::shared_ptr<std::promise<void>> wait_for_signal,
                     MockedResponseInformation response_information,
                     std::shared_ptr<std::promise<void>> post_signal =
                         std::make_shared<std::promise<void>>());
