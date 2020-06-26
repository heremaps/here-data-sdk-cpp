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

#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/NetworkMock.h>
#include <olp/authentication/AuthenticationClient.h>
#include <olp/authentication/AutoRefreshingToken.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/porting/warning_disable.h>

#include "AuthenticationMockedResponses.h"

//  OLPEDGE-1797
PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

namespace auth = olp::authentication;
using testing::_;

namespace {
const std::string kErrorOk = "OK";
const std::string kTimestampUrl = R"(https://account.api.here.com/timestamp)";

auth::TokenEndpoint::TokenResponse GetTokenFromSyncRequest(
    olp::client::CancellationToken& cancellationToken,
    const auth::AutoRefreshingToken& autoToken,
    const std::chrono::seconds minimumValidity =
        auth::kDefaultMinimumValiditySeconds) {
  return autoToken.GetToken(cancellationToken, minimumValidity);
}

auth::TokenEndpoint::TokenResponse GetTokenFromAsyncRequest(
    olp::client::CancellationToken& cancellationToken,
    const auth::AutoRefreshingToken& autoToken,
    const std::chrono::seconds minimumValidity =
        auth::kDefaultMinimumValiditySeconds) {
  std::promise<auth::TokenEndpoint::TokenResponse> promise;
  auto future = promise.get_future();
  cancellationToken = autoToken.GetToken(
      [&promise](auth::TokenEndpoint::TokenResponse tokenResponse) {
        promise.set_value(tokenResponse);
      },
      minimumValidity);
  return future.get();
}

void TestAutoRefreshingTokenCancel(
    auth::TokenEndpoint& token_endpoint,
    std::function<auth::TokenEndpoint::TokenResponse(
        olp::client::CancellationToken& cancellationToken,
        const auth::AutoRefreshingToken& autoToken,
        const std::chrono::seconds minimumValidity)>
        func) {
  auto autoToken = token_endpoint.RequestAutoRefreshingToken();

  std::thread threads[2];
  auto tokenResponses = std::vector<auth::TokenEndpoint::TokenResponse>();
  std::mutex tokenResponsesMutex;
  olp::client::CancellationToken cancellationToken;

  // get a first refresh token and wait for it to come back.
  tokenResponses.emplace_back(
      func(cancellationToken, autoToken, std::chrono::minutes(5)));
  ASSERT_EQ(tokenResponses.size(), 1u);

  // get a second forced refresh token
  threads[0] = std::thread([&]() {
    std::lock_guard<std::mutex> guard(tokenResponsesMutex);
    tokenResponses.emplace_back(
        func(cancellationToken, autoToken, auth::kForceRefresh));
  });

  // Cancel the refresh from another thread so that the response will come
  // back with the same old token
  threads[1] = std::thread([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cancellationToken.Cancel();
  });
  threads[0].join();
  threads[1].join();

  ASSERT_EQ(tokenResponses.size(), 2u);
  ASSERT_EQ(tokenResponses[0].GetResult().GetAccessToken(),
            tokenResponses[1].GetResult().GetAccessToken());
  ASSERT_LE(std::abs(tokenResponses[1].GetResult().GetExpiryTime() -
                     tokenResponses[0].GetResult().GetExpiryTime()),
            10ll);
}

}  // namespace

class HereAccountOauth2Test : public ::testing::Test {
 public:
  HereAccountOauth2Test() : key_("key"), secret_("secret") {}
  void SetUp() {
    network_ = std::make_shared<NetworkMock>();
    task_scheduler_ =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();

    auth::AuthenticationSettings settings;
    settings.network_request_handler = network_;
    settings.task_scheduler = task_scheduler_;
    settings.use_system_time = true;
    settings.token_endpoint_url = "https://authentication.server.url";

    client_ = std::make_unique<auth::AuthenticationClient>(settings);
  }

  void TearDown() {
    network_.reset();
    client_.reset();
  }

 protected:
  std::shared_ptr<NetworkMock> network_;
  std::unique_ptr<olp::authentication::AuthenticationClient> client_;
  std::shared_ptr<olp::thread::TaskScheduler> task_scheduler_;
  const std::string key_;
  const std::string secret_;
};

TEST_F(HereAccountOauth2Test, AutoRefreshingTokenCancelSync) {
  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(2)
      .WillRepeatedly(
          [&](olp::http::NetworkRequest /*request*/,
              olp::http::Network::Payload payload,
              olp::http::Network::Callback callback,
              olp::http::Network::HeaderCallback /*header_callback*/,
              olp::http::Network::DataCallback data_callback) {
            olp::http::RequestId request_id(5);
            if (payload) {
              *payload << kResponseValidJson;
            }
            callback(olp::http::NetworkResponse()
                         .WithRequestId(request_id)
                         .WithStatus(olp::http::HttpStatusCode::OK)
                         .WithError(kErrorOk));
            if (data_callback) {
              auto raw = const_cast<char*>(kResponseValidJson.c_str());
              data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                            kResponseValidJson.size());
            }

            return olp::http::SendOutcome(request_id);
          });

  auth::Settings settings({key_, secret_});
  settings.network_request_handler = network_;
  auth::TokenEndpoint token_endpoint(settings);

  TestAutoRefreshingTokenCancel(
      token_endpoint, [](olp::client::CancellationToken& cancellationToken,
                         const auth::AutoRefreshingToken& autoToken,
                         const std::chrono::seconds minimumValidity) {
        return GetTokenFromSyncRequest(cancellationToken, autoToken,
                                       minimumValidity)
            .MoveResult();
      });
}

TEST_F(HereAccountOauth2Test, AutoRefreshingTokenBackendError) {
  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .WillOnce([&](olp::http::NetworkRequest /*request*/,
                    olp::http::Network::Payload payload,
                    olp::http::Network::Callback callback,
                    olp::http::Network::HeaderCallback /*header_callback*/,
                    olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << kResponseUnauthorized;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::UNAUTHORIZED)
                     .WithError(kErrorOk));
        if (data_callback) {
          auto raw = const_cast<char*>(kResponseUnauthorized.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                        kResponseUnauthorized.size());
        }

        return olp::http::SendOutcome(request_id);
      });

  auth::Settings settings({key_, secret_});
  settings.network_request_handler = network_;
  auth::TokenEndpoint token_endpoint(settings);
  olp::client::CancellationToken cancellationToken;

  auto token = GetTokenFromSyncRequest(
      cancellationToken, token_endpoint.RequestAutoRefreshingToken(),
      auth::kDefaultMinimumValiditySeconds);

  EXPECT_TRUE(token.IsSuccessful());
  EXPECT_NE(token.GetResult().GetErrorResponse().code, 0);
  EXPECT_EQ(token.GetResult().GetHttpStatus(),
            olp::http::HttpStatusCode::UNAUTHORIZED);
}

TEST_F(HereAccountOauth2Test, AutoRefreshingTokenCancelAsync) {
  EXPECT_CALL(*network_, Send(_, _, _, _, _))
      .Times(2)
      .WillRepeatedly(
          [&](olp::http::NetworkRequest /*request*/,
              olp::http::Network::Payload payload,
              olp::http::Network::Callback callback,
              olp::http::Network::HeaderCallback /*header_callback*/,
              olp::http::Network::DataCallback data_callback) {
            olp::http::RequestId request_id(5);
            if (payload) {
              *payload << kResponseValidJson;
            }
            callback(olp::http::NetworkResponse()
                         .WithRequestId(request_id)
                         .WithStatus(olp::http::HttpStatusCode::OK)
                         .WithError(kErrorOk));
            if (data_callback) {
              auto raw = const_cast<char*>(kResponseValidJson.c_str());
              data_callback(reinterpret_cast<uint8_t*>(raw), 0,
                            kResponseValidJson.size());
            }

            return olp::http::SendOutcome(request_id);
          });

  auth::Settings settings({key_, secret_});
  settings.network_request_handler = network_;
  auth::TokenEndpoint token_endpoint(settings);

  TestAutoRefreshingTokenCancel(
      token_endpoint, [](olp::client::CancellationToken& cancellationToken,
                         const auth::AutoRefreshingToken& autoToken,
                         const std::chrono::seconds minimumValidity) {
        return GetTokenFromAsyncRequest(cancellationToken, autoToken,
                                        minimumValidity)
            .MoveResult();
      });
}

PORTING_POP_WARNINGS()
