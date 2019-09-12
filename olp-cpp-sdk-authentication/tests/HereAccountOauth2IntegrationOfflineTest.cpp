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

#include <gtest/gtest.h>

#include <olp/authentication/AuthenticationClient.h>
#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/AutoRefreshingToken.h>
#include <olp/authentication/ErrorResponse.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenEndpoint.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/authentication/TokenRequest.h>
#include <olp/authentication/TokenResult.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/NetworkProxySettings.h>
#include "AuthenticationOfflineTest.h"

using namespace olp::authentication;

using testing::_;

namespace {
constexpr auto kTestMaxExecutionTime = std::chrono::seconds(30);

TokenEndpoint::TokenResponse GetTokenFromSyncRequest(
    const AutoRefreshingToken& autoToken,
    const std::chrono::seconds minimumValidity =
        kDefaultMinimumValiditySeconds) {
  return autoToken.GetToken(minimumValidity);
}

TokenEndpoint::TokenResponse GetTokenFromSyncRequest(
    olp::client::CancellationToken& cancellationToken,
    const AutoRefreshingToken& autoToken,
    const std::chrono::seconds minimumValidity =
        kDefaultMinimumValiditySeconds) {
  return autoToken.GetToken(cancellationToken, minimumValidity);
}

TokenEndpoint::TokenResponse getTokenFromAsyncRequest(
    const AutoRefreshingToken& autoToken,
    const std::chrono::seconds minimumValidity =
        kDefaultMinimumValiditySeconds) {
  std::promise<TokenEndpoint::TokenResponse> promise;
  auto future = promise.get_future();
  autoToken.GetToken(
      [&promise](TokenEndpoint::TokenResponse tokenResponse) {
        promise.set_value(tokenResponse);
      },
      minimumValidity);
  return future.get();
}

TokenEndpoint::TokenResponse getTokenFromAsyncRequest(
    olp::client::CancellationToken& cancellationToken,
    const AutoRefreshingToken& autoToken,
    const std::chrono::seconds minimumValidity =
        kDefaultMinimumValiditySeconds) {
  std::promise<TokenEndpoint::TokenResponse> promise;
  auto future = promise.get_future();
  cancellationToken = autoToken.GetToken(
      [&promise](TokenEndpoint::TokenResponse tokenResponse) {
        promise.set_value(tokenResponse);
      },
      minimumValidity);
  return future.get();
}

void TestAutoRefreshingTokenValidRequest(
    TokenEndpoint& token_endpoint, std::function<TokenEndpoint::TokenResponse(
                                       const AutoRefreshingToken& autoToken)>
                                       func) {
  auto tokenResponse = func(token_endpoint.RequestAutoRefreshingToken());
  EXPECT_TRUE(tokenResponse.IsSuccessful());
  EXPECT_GT(tokenResponse.GetResult().GetAccessToken().length(), 42u);
  EXPECT_GT(tokenResponse.GetResult().GetExpiryTime(), time(nullptr));
}

void TestAutoRefreshingTokenCancel(
    TokenEndpoint& token_endpoint,
    std::function<TokenEndpoint::TokenResponse(
        olp::client::CancellationToken& cancellationToken,
        const AutoRefreshingToken& autoToken,
        const std::chrono::seconds minimumValidity)>
        func) {
  auto autoToken = token_endpoint.RequestAutoRefreshingToken();

  std::thread threads[2];
  auto tokenResponses = std::vector<TokenEndpoint::TokenResponse>();
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
        func(cancellationToken, autoToken, kForceRefresh));
  });

  // Cancel the refresh from another thread so that the response will come back
  // with the same old token
  threads[1] = std::thread([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cancellationToken.cancel();
  });
  threads[0].join();
  threads[1].join();

  ASSERT_EQ(tokenResponses.size(), 2u);
  ASSERT_EQ(tokenResponses[0].GetResult().GetAccessToken(),
            tokenResponses[1].GetResult().GetAccessToken());
  ASSERT_LE(abs(tokenResponses[1].GetResult().GetExpiryTime() -
                tokenResponses[0].GetResult().GetExpiryTime()),
            10);
}

}  // namespace

class TestHereAccountOauth2IntegrationOffline
    : public AuthenticationOfflineTest {};

TEST_F(TestHereAccountOauth2IntegrationOffline, AutoRefreshingTokenCancelSync) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .Times(2)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << response_1;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::OK)
                     .WithError(ERROR_OK));
        if (data_callback) {
          auto raw = const_cast<char*>(response_1.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0, response_1.size());
        }

        return olp::http::SendOutcome(request_id);
      });

  Settings settings;
  settings.network_request_handler = network_mock_;
  TokenEndpoint token_endpoint(
      AuthenticationCredentials(
          CustomParameters::getArgument("integration_production_service_id"),
          CustomParameters::getArgument(
              "integration_production_service_secret")),
      settings);

  TestAutoRefreshingTokenCancel(
      token_endpoint, [](olp::client::CancellationToken& cancellationToken,
                         const AutoRefreshingToken& autoToken,
                         const std::chrono::seconds minimumValidity) {
        return GetTokenFromSyncRequest(cancellationToken, autoToken,
                                       minimumValidity)
            .GetResult();
      });
}

TEST_F(TestHereAccountOauth2IntegrationOffline,
       AutoRefreshingTokenCancelAsync) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .Times(2)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << response_1;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::OK)
                     .WithError(ERROR_OK));
        if (data_callback) {
          auto raw = const_cast<char*>(response_1.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0, response_1.size());
        }

        return olp::http::SendOutcome(request_id);
      });

  Settings settings;
  settings.network_request_handler = network_mock_;
  TokenEndpoint token_endpoint(
      AuthenticationCredentials(
          CustomParameters::getArgument("integration_production_service_id"),
          CustomParameters::getArgument(
              "integration_production_service_secret")),
      settings);

  TestAutoRefreshingTokenCancel(
      token_endpoint, [](olp::client::CancellationToken& cancellationToken,
                         const AutoRefreshingToken& autoToken,
                         const std::chrono::seconds minimumValidity) {
        return getTokenFromAsyncRequest(cancellationToken, autoToken,
                                        minimumValidity)
            .GetResult();
      });
}
