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

#include <chrono>
#include <future>

#include <gmock/gmock.h>
#include <olp/core/http/Network.h>

namespace olp {
namespace tests {
namespace common {

using NetworkCallback = std::function<http::SendOutcome(
    http::NetworkRequest, http::Network::Payload, http::Network::Callback,
    http::Network::HeaderCallback, http::Network::DataCallback)>;

using CancelCallback = std::function<void(http::RequestId)>;

class NetworkMock : public http::Network {
 public:
  NetworkMock();

  ~NetworkMock() override;

  MOCK_METHOD(http::SendOutcome, Send,
              (http::NetworkRequest request, http::Network::Payload payload,
               http::Network::Callback callback,
               http::Network::HeaderCallback header_callback,
               http::Network::DataCallback data_callback),
              (override));

  MOCK_METHOD(void, Cancel, (http::RequestId id), (override));
};

/**
 * @brief Data Structure which is used by network mock to fill response's body
 * on network request.
 */
struct MockedResponseInformation {
  int status;        /// HTTP status code for response.
  const char* data;  /// Body of HTTP response.
  http::Headers headers;
  MockedResponseInformation(int status, const char* data,
                            http::Headers&& headers = {})
      : status(status), data(data), headers(std::move(headers)) {}
};

/**
 * @brief Helper function creates actions that can be provided to the
 * NetworkMock instance.
 *
 * @param pre_signal - promise that will notify the test that it has reached
 * network code.
 * @param wait_for_signal - promise that test should set to let network mock
 * know that it is time to check requets for cancellation. Test needs to cancel
 * request before setting this promise.
 * @param response_information - Data that network mock should return in
 * response if request wasn't cancelled.
 * @param post_signal - optional promise that network mock will set after
 * request is finished.
 *
 * @return Triple: RequestId; Action for method Send(); Action for method
 * Cancel();
 */
std::tuple<http::RequestId, NetworkCallback, CancelCallback>
GenerateNetworkMockActions(std::shared_ptr<std::promise<void>> pre_signal,
                           std::shared_ptr<std::promise<void>> wait_for_signal,
                           MockedResponseInformation response_information,
                           std::shared_ptr<std::promise<void>> post_signal =
                               std::make_shared<std::promise<void>>());

///
/// NetworkMock Actions
///

/**
 * @brief Helper function creates Network::Send mock function that returns a
 * specified response, body, headers after specified delay from a different
 * thread.
 *
 * @param response Network response.
 * @param response_body Response payload.
 * @param headers Response headers (optional).
 * @param delay Response delay (optional).
 *
 * @return A function that mimics Network::Send method.
 */
NetworkCallback ReturnHttpResponse(
    http::NetworkResponse response, const std::string& response_body,
    const http::Headers& headers = {},
    std::chrono::nanoseconds delay = std::chrono::nanoseconds(0));

inline http::NetworkResponse GetResponse(int status) {
  return http::NetworkResponse().WithStatus(status);
}

}  // namespace common
}  // namespace tests
}  // namespace olp
