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

#include <future>
#include <memory>

#include <gtest/gtest.h>
#include <olp/authentication/AuthenticationClient.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/Network.h>
#include <olp/core/porting/make_unique.h>
#include <testutils/CustomParameters.hpp>
#include "TestConstants.h"

using namespace ::olp::authentication;

namespace {

class AuthenticationProductionTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    s_network_ = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();
  }

  static void TearDownTestSuite() { s_network_.reset(); }

  void SetUp() override {
    // Use production HERE Account server
    client_ = std::make_unique<AuthenticationClient>();
    client_->SetTaskScheduler(
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler());
    client_->SetNetwork(s_network_);
  }

  void TearDown() override { client_.reset(); }

 protected:
  std::unique_ptr<AuthenticationClient> client_;

  static std::shared_ptr<olp::http::Network> s_network_;
};

// Static network instance is necessary as it needs to outlive any created
// clients. This is a known limitation as triggered send requests capture the
// network instance inside the callbacks.
std::shared_ptr<olp::http::Network> AuthenticationProductionTest::s_network_;

TEST_F(AuthenticationProductionTest, SignInClient) {
  AuthenticationCredentials credentials(
      CustomParameters::getArgument("production_service_id"),
      CustomParameters::getArgument("production_service_secret"));
  std::promise<AuthenticationClient::SignInClientResponse> request;
  auto request_future = request.get_future();
  client_->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request.set_value(response);
      },
      std::chrono::seconds(kExpiryTime));

  AuthenticationClient::SignInClientResponse response = request_future.get();
  std::time_t now = std::time(nullptr);
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response.GetResult().GetStatus());
  EXPECT_STREQ(kErrorOk.c_str(),
               response.GetResult().GetErrorResponse().message.c_str());
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + kMaxExpiry, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinExpiry, response.GetResult().GetExpiryTime());
  EXPECT_FALSE(response.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());

  std::promise<AuthenticationClient::SignInClientResponse> request_2;
  now = std::time(nullptr);
  auto request_future_2 = request_2.get_future();
  client_->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request_2.set_value(response);
      },
      std::chrono::seconds(kExtendedExpiryTime));

  AuthenticationClient::SignInClientResponse response_2 =
      request_future_2.get();
  EXPECT_TRUE(response_2.IsSuccessful());
  EXPECT_FALSE(response_2.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + kMaxExtendedExpiry, response_2.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinExtendedExpiry, response_2.GetResult().GetExpiryTime());
  EXPECT_FALSE(response_2.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response_2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_2.GetResult().GetUserIdentifier().empty());

  std::promise<AuthenticationClient::SignInClientResponse> request_3;
  now = std::time(nullptr);
  auto request_future_3 = request_3.get_future();
  client_->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request_3.set_value(response);
      },
      std::chrono::seconds(kCustomExpiryTime));

  AuthenticationClient::SignInClientResponse response_3 =
      request_future_3.get();
  EXPECT_TRUE(response_3.IsSuccessful());
  EXPECT_FALSE(response_3.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + kMaxCustomExpiry, response_3.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinCustomExpiry, response_3.GetResult().GetExpiryTime());
  EXPECT_FALSE(response_3.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response_3.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_3.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationProductionTest, SignInClientMaxExpiration) {
  // Test maximum token expiration 24 h
  AuthenticationCredentials credentials(
      CustomParameters::getArgument("production_service_id"),
      CustomParameters::getArgument("production_service_secret"));
  std::promise<AuthenticationClient::SignInClientResponse> request;
  auto request_future = request.get_future();
  time_t now = std::time(nullptr);
  client_->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request.set_value(response);
      });

  AuthenticationClient::SignInClientResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + kMaxLimitExpiry, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinLimitExpiry, response.GetResult().GetExpiryTime());

  // Test token expiration greater than 24h
  std::promise<AuthenticationClient::SignInClientResponse> request_2;
  auto request_future_2 = request_2.get_future();
  now = std::time(nullptr);
  client_->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request_2.set_value(response);
      },
      std::chrono::seconds(90000));

  AuthenticationClient::SignInClientResponse response_2 =
      request_future_2.get();
  EXPECT_TRUE(response_2.IsSuccessful());
  EXPECT_FALSE(response_2.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + kMaxLimitExpiry, response_2.GetResult().GetExpiryTime());
  EXPECT_LT(now + kMinLimitExpiry, response_2.GetResult().GetExpiryTime());
  EXPECT_FALSE(response_2.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response_2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_2.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationProductionTest, InvalidCredentials) {
  AuthenticationCredentials credentials(
      CustomParameters::getArgument("production_service_id"),
      CustomParameters::getArgument("production_service_id"));
  std::promise<AuthenticationClient::SignInClientResponse> request;
  auto request_future = request.get_future();
  client_->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request.set_value(response);
      });

  AuthenticationClient::SignInClientResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(kErrorUnauthorizedCode,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(kErrorUnauthorizedMessage,
            response.GetResult().GetErrorResponse().message);
  EXPECT_TRUE(response.GetResult().GetAccessToken().empty());
  EXPECT_TRUE(response.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());
}

}  // namespace
