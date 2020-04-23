/*
 * Copyright (C) 2020 HERE Europe B.V.
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
#include <chrono>
#include <string>

#include <olp/authentication/DecisionClient.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <testutils/CustomParameters.hpp>

#include "Utils.h"

using namespace testing;

namespace {

class DecisionClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();

    auto appid = CustomParameters::getArgument("service_id");
    auto secret = CustomParameters::getArgument("service_secret");

    olp::authentication::Settings auth_settings({appid, secret});
    auth_settings.network_request_handler = network;

    olp::authentication::TokenProviderDefault provider(auth_settings);
    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;

    settings_ = std::make_shared<olp::client::OlpClientSettings>();
    settings_->network_request_handler = network;
    settings_->authentication_settings = auth_client_settings;
  }

  void TearDown() override {
    auto network = std::move(settings_->network_request_handler);
    settings_.reset();
  }

  std::shared_ptr<olp::client::OlpClientSettings> settings_;
};

TEST_F(DecisionClientTest, GetDecisionAllow) {
  olp::authentication::DecisionClient client(*settings_);

  auto request = olp::authentication::AuthorizeRequest().WithServiceId(
      "SERVICE-fc0561eb-7098-449d-8cbe-12f08e5474e0");
  request.WithAction("getTileCore");
    auto future = client.GetDecision(request);
    auto response = future.GetFuture().get();

    EXPECT_TRUE(response.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetClientId(), "9PBigz3zyXks0OlAQv13");
    ASSERT_EQ(response.GetResult().GetDecision(),
              olp::authentication::DecisionType::kAllow);
}

TEST_F(DecisionClientTest, GetDecisionFailed) {
  olp::authentication::DecisionClient client(*settings_);

  auto request =
      olp::authentication::AuthorizeRequest().WithServiceId("Wrong_service");
  request.WithAction("getTileCore");
    auto future = client.GetDecision(request);
    auto response = future.GetFuture().get();

    EXPECT_TRUE(response.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetClientId(), "9PBigz3zyXks0OlAQv13");
    ASSERT_EQ(response.GetResult().GetDecision(),
              olp::authentication::DecisionType::kDeny);
}

TEST_F(DecisionClientTest, GetDecisionWithTwoActions) {
  olp::authentication::DecisionClient client(*settings_);

  auto request =
      olp::authentication::AuthorizeRequest()
          .WithServiceId("SERVICE-fc0561eb-7098-449d-8cbe-12f08e5474e0")
          .WithAction("getTileCore")
          .WithAction("InvalidAction")
          .WithOperatorType(
              olp::authentication::AuthorizeRequest::DecisionOperatorType::kOr)
          .WithDiagnostics(true);

  auto future = client.GetDecision(request);
  auto response = future.GetFuture().get();

  EXPECT_TRUE(response.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetClientId(), "9PBigz3zyXks0OlAQv13");
  ASSERT_EQ(response.GetResult().GetDecision(),
            olp::authentication::DecisionType::kAllow);
  auto it = response.GetResult().GetActionResults().begin();
  ASSERT_EQ(it->GetDecision(), olp::authentication::DecisionType::kAllow);
  ASSERT_EQ(it->GetPermitions().front().first, "getTileCore");
  ASSERT_EQ(it->GetPermitions().front().second,
            olp::authentication::DecisionType::kAllow);
  ASSERT_EQ((++it)->GetDecision(), olp::authentication::DecisionType::kDeny);
  EXPECT_TRUE(it->GetPermitions().empty());
}

}  // namespace
