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

#include "NetworkMock.h"

#include <thread>

namespace olp {
namespace tests {
namespace common {

NetworkMock::NetworkMock() = default;

NetworkMock::~NetworkMock() = default;

std::tuple<olp::http::RequestId, NetworkCallback, CancelCallback>
GenerateNetworkMockActions(std::shared_ptr<std::promise<void>> pre_signal,
                           std::shared_ptr<std::promise<void>> wait_for_signal,
                           MockedResponseInformation response_information,
                           std::shared_ptr<std::promise<void>> post_signal) {
  using namespace olp::http;

  static std::atomic<RequestId> s_request_id{
      static_cast<RequestId>(RequestIdConstants::RequestIdMin)};

  olp::http::RequestId request_id = s_request_id.fetch_add(1);

  auto completed = std::make_shared<std::atomic_bool>(false);

  // callback is generated when the send method is executed, in order to receive
  // the cancel callback, we need to pass it to store it somewhere and share
  // with cancel mock.
  auto callback_placeholder = std::make_shared<olp::http::Network::Callback>();

  auto mocked_send =
      [request_id, completed, pre_signal, wait_for_signal, response_information,
       post_signal, callback_placeholder](
          NetworkRequest request, Network::Payload payload,
          Network::Callback callback, Network::HeaderCallback header_callback,
          Network::DataCallback /*data_callback*/) -> olp::http::SendOutcome {
    *callback_placeholder = callback;

    auto mocked_network_block = [request, pre_signal, wait_for_signal,
                                 completed, header_callback, callback,
                                 response_information, post_signal, payload]() {
      // emulate a small response delay
      std::this_thread::sleep_for(std::chrono::milliseconds(50));

      // notify waiting thread that we reached the network code
      pre_signal->set_value();

      // wait until test cancel request during execution
      wait_for_signal->get_future().get();

      // in the case request was not canceled return the expected payload
      if (!completed->exchange(true)) {
        const auto data_len = strlen(response_information.data);
        payload->write(response_information.data, data_len);
        for (const auto& header : response_information.headers) {
          header_callback(header.first, header.second);
        }
        callback(NetworkResponse().WithStatus(response_information.status));
      }

      // notify that request finished
      post_signal->set_value();
    };

    // simulate that network code is actually running in the background.
    std::thread(std::move(mocked_network_block)).detach();

    return SendOutcome(request_id);
  };

  auto mocked_cancel = [completed,
                        callback_placeholder](olp::http::RequestId /*id*/) {
    if (!completed->exchange(true)) {
      auto cancel_code = static_cast<int>(ErrorCode::CANCELLED_ERROR);
      (*callback_placeholder)(
          NetworkResponse().WithError("Cancelled").WithStatus(cancel_code));
    }
  };

  return std::make_tuple(request_id, std::move(mocked_send),
                         std::move(mocked_cancel));
}

///
/// NetworkMock Actions
///

NetworkCallback ReturnHttpResponse(olp::http::NetworkResponse response,
                                   const std::string& response_body,
                                   const http::Headers& headers) {
  return [=](olp::http::NetworkRequest /*request*/,
             olp::http::Network::Payload payload,
             olp::http::Network::Callback callback,
             olp::http::Network::HeaderCallback header_callback,
             olp::http::Network::DataCallback /*data_callback*/)
             -> olp::http::SendOutcome {
    std::thread([=]() {
      for (const auto& header : headers) {
        header_callback(header.first, header.second);
      }
      *payload << response_body;
      callback(response);
    })
        .detach();

    constexpr auto unused_request_id = 5;
    return olp::http::SendOutcome(unused_request_id);
  };
}

}  // namespace common
}  // namespace tests
}  // namespace olp
