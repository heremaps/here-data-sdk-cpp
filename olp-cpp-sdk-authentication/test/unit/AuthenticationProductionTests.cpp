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
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/network/Network.h>
#include <olp/core/porting/make_unique.h>
#include <future>
#include <memory>
#include "CommonTestUtils.h"
#include "testutils/CustomParameters.hpp"

static const unsigned int EXPIRY_TIME = 3600;
static const unsigned int MAX_EXPIRY = EXPIRY_TIME + 10;
static const unsigned int MIN_EXPIRY = EXPIRY_TIME - 10;

static const unsigned int CUSTOM_EXPIRY_TIME = 6000;
static const unsigned int MAX_CUSTOM_EXPIRY = CUSTOM_EXPIRY_TIME + 10;
static const unsigned int MIN_CUSTOM_EXPIRY = CUSTOM_EXPIRY_TIME - 10;

static const unsigned int EXTENDED_EXPIRY_TIME = 2 * EXPIRY_TIME;
static const unsigned int MAX_EXTENDED_EXPIRY = EXTENDED_EXPIRY_TIME + 10;
static const unsigned int MIN_EXTENDED_EXPIRY = EXTENDED_EXPIRY_TIME - 10;

static const unsigned int LIMIT_EXPIRY = 86400;
static const unsigned int MAX_LIMIT_EXPIRY = LIMIT_EXPIRY + 10;
static const unsigned int MIN_LIMIT_EXPIRY = LIMIT_EXPIRY - 10;

using namespace olp::authentication;

class AuthenticationOnlineProductionTest : public ::testing::Test {
 public:
  static void SetUpTestSuite() {
    s_network_ = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler(1);
  }

  static void TearDownTestSuite() { s_network_.reset(); }

  void SetUp() override {
    // Use production HERE Account server
    client = std::make_unique<AuthenticationClient>();
    client->SetTaskScheduler(
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler());
    client->SetNetwork(s_network_);
  }

  void TearDown() override { client.reset(); }

 public:
  std::unique_ptr<AuthenticationClient> client;

  static std::shared_ptr<olp::http::Network> s_network_;
};

std::shared_ptr<olp::http::Network> AuthenticationOnlineProductionTest::s_network_;

TEST_F(AuthenticationOnlineProductionTest, SignInClient) {
  AuthenticationCredentials credentials(
      CustomParameters::getArgument("production_service_id"),
      CustomParameters::getArgument("production_service_secret"));
  std::promise<AuthenticationClient::SignInClientResponse> request;
  auto request_future = request.get_future();
  client->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request.set_value(response);
      },
      std::chrono::seconds(EXPIRY_TIME));
  request_future.wait();

  AuthenticationClient::SignInClientResponse response = request_future.get();
  std::time_t now = std::time(nullptr);
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response.GetResult().GetStatus());
  EXPECT_STREQ(ERROR_OK.c_str(),
               response.GetResult().GetErrorResponse().message.c_str());
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + MAX_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_FALSE(response.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());

  std::promise<AuthenticationClient::SignInClientResponse> request_2;
  now = std::time(nullptr);
  auto request_future_2 = request_2.get_future();
  client->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request_2.set_value(response);
      },
      std::chrono::seconds(EXTENDED_EXPIRY_TIME));
  request_future_2.wait();

  AuthenticationClient::SignInClientResponse response_2 =
      request_future_2.get();
  EXPECT_TRUE(response_2.IsSuccessful());
  EXPECT_FALSE(response_2.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + MAX_EXTENDED_EXPIRY, response_2.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_EXTENDED_EXPIRY, response_2.GetResult().GetExpiryTime());
  EXPECT_FALSE(response_2.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response_2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_2.GetResult().GetUserIdentifier().empty());

  std::promise<AuthenticationClient::SignInClientResponse> request_3;
  now = std::time(nullptr);
  auto request_future_3 = request_3.get_future();
  client->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request_3.set_value(response);
      },
      std::chrono::seconds(CUSTOM_EXPIRY_TIME));
  request_future_3.wait();

  AuthenticationClient::SignInClientResponse response_3 =
      request_future_3.get();
  EXPECT_TRUE(response_3.IsSuccessful());
  EXPECT_FALSE(response_3.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + MAX_CUSTOM_EXPIRY, response_3.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_CUSTOM_EXPIRY, response_3.GetResult().GetExpiryTime());
  EXPECT_FALSE(response_3.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response_3.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_3.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationOnlineProductionTest, SignInClientMaxExpiration) {
  // Test maximum token expiration 24 h
  AuthenticationCredentials credentials(
      CustomParameters::getArgument("production_service_id"),
      CustomParameters::getArgument("production_service_secret"));
  std::promise<AuthenticationClient::SignInClientResponse> request;
  auto request_future = request.get_future();
  time_t now = std::time(nullptr);
  client->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInClientResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + MAX_LIMIT_EXPIRY, response.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_LIMIT_EXPIRY, response.GetResult().GetExpiryTime());

  // Test token expiration greater than 24h
  std::promise<AuthenticationClient::SignInClientResponse> request_2;
  auto request_future_2 = request_2.get_future();
  now = std::time(nullptr);
  client->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request_2.set_value(response);
      },
      std::chrono::seconds(90000));
  request_future_2.wait();

  AuthenticationClient::SignInClientResponse response_2 =
      request_future_2.get();
  EXPECT_TRUE(response_2.IsSuccessful());
  EXPECT_FALSE(response_2.GetResult().GetAccessToken().empty());
  EXPECT_GE(now + MAX_LIMIT_EXPIRY, response_2.GetResult().GetExpiryTime());
  EXPECT_LT(now + MIN_LIMIT_EXPIRY, response_2.GetResult().GetExpiryTime());
  EXPECT_FALSE(response_2.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response_2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response_2.GetResult().GetUserIdentifier().empty());
}

TEST_F(AuthenticationOnlineProductionTest, InvalidCredentials) {
  AuthenticationCredentials credentials(
      CustomParameters::getArgument("production_service_id"),
      CustomParameters::getArgument("production_service_id"));
  std::promise<AuthenticationClient::SignInClientResponse> request;
  auto request_future = request.get_future();
  client->SignInClient(
      credentials,
      [&](const AuthenticationClient::SignInClientResponse& response) {
        request.set_value(response);
      });
  request_future.wait();

  AuthenticationClient::SignInClientResponse response = request_future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(ERROR_UNAUTHORIZED_CODE,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_UNAUTHORIZED_MESSAGE,
            response.GetResult().GetErrorResponse().message);
  EXPECT_TRUE(response.GetResult().GetAccessToken().empty());
  EXPECT_TRUE(response.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());
}
