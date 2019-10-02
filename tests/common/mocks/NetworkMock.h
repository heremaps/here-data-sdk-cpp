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

#include <gmock/gmock.h>
#include <olp/core/http/Network.h>

class NetworkMock : public olp::http::Network {
 public:
  NetworkMock();

  ~NetworkMock() override;

  MOCK_METHOD(olp::http::SendOutcome, Send,
              (olp::http::NetworkRequest request,
               olp::http::Network::Payload payload,
               olp::http::Network::Callback callback,
               olp::http::Network::HeaderCallback header_callback,
               olp::http::Network::DataCallback data_callback),
              (override));

  MOCK_METHOD(void, Cancel, (olp::http::RequestId id), (override));

  static std::function<olp::http::SendOutcome(
      olp::http::NetworkRequest request, olp::http::Network::Payload payload,
      olp::http::Network::Callback callback,
      olp::http::Network::HeaderCallback header_callback,
      olp::http::Network::DataCallback data_callback)>
  ReturnHttpResponse(olp::http::NetworkResponse response,
                     const std::string& response_body);
};
