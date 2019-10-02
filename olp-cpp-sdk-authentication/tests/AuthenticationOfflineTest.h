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

#include "AuthenticationBaseTest.h"
#include <mocks/NetworkMock.h>

class AuthenticationOfflineTest : public AuthenticationBaseTest {
 public:
  void SetUp() override {
    AuthenticationBaseTest::SetUp();

    network_mock_ = std::make_shared<NetworkMock>();
    network_ = network_mock_;
    task_scheduler_ =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
    client_->SetNetwork(network_);
    client_->SetTaskScheduler(task_scheduler_);
  }

  void TearDown() override { AuthenticationBaseTest::TearDown(); }

  void ExecuteSigninRequest(int http, int http_result,
                            const std::string& error_message,
                            const std::string& data = "", int error_code = 0) {
    AuthenticationCredentials credentials(id_, secret_);
    std::promise<AuthenticationClient::SignInClientResponse> request;
    auto request_future = request.get_future();

    EXPECT_CALL(*network_mock_, Send(testing::_, testing::_, testing::_,
                                     testing::_, testing::_))
        .Times(1)
        .WillOnce([&](olp::http::NetworkRequest request,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback header_callback,
                      olp::http::Network::DataCallback data_callback) {
          olp::http::RequestId request_id(5);
          if (payload) {
            *payload << data;
          }
          callback(olp::http::NetworkResponse()
                       .WithRequestId(request_id)
                       .WithStatus(http));
          if (data_callback) {
            auto raw = const_cast<char*>(data.c_str());
            data_callback(reinterpret_cast<uint8_t*>(raw), 0, data.size());
          }

          return olp::http::SendOutcome(request_id);
        });

    client_->SignInClient(
        credentials,
        [&](const AuthenticationClient::SignInClientResponse& response) {
          request.set_value(response);
        });
    request_future.wait();
    AuthenticationClient::SignInClientResponse response = request_future.get();
    if (response.IsSuccessful()) {
      EXPECT_EQ(http_result, response.GetResult().GetStatus());
      EXPECT_EQ(error_message, response.GetResult().GetErrorResponse().message);
      if (error_code != 0) {
        EXPECT_EQ(error_code, response.GetResult().GetErrorResponse().code);
      }
    }
  }

 protected:
  std::shared_ptr<NetworkMock> network_mock_;
};
