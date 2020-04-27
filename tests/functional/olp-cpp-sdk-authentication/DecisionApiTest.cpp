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

#include <olp/authentication/AuthenticationClient.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/Network.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/porting/warning_disable.h>
#include <testutils/CustomParameters.hpp>
#include "AuthenticationTestUtils.h"
#include "TestConstants.h"

namespace {

constexpr auto kLogTag = "AuthenticationClientTestAuthorize";

using namespace ::olp::authentication;

class AuthenticationClientTestAuthorize : public ::testing::Test {
 protected:
  std::string id_;
  std::string secret_;
  std::string service_id_;
  std::shared_ptr<olp::authentication::AuthenticationClient> client_;
  std::shared_ptr<olp::http::Network> network_;
  std::shared_ptr<olp::thread::TaskScheduler> task_scheduler_;

 protected:
  void SetUp() override {
    id_ = CustomParameters::getArgument("decision_api_test_appid");
    secret_ = CustomParameters::getArgument("decision_api_test_secret");
    service_id_ = CustomParameters::getArgument("decision_api_test_service_id");
    network_ = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler(1);
    task_scheduler_ =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();

    AuthenticationSettings settings;
    settings.network_request_handler = network_;
    settings.task_scheduler = task_scheduler_;

    client_ = std::make_unique<AuthenticationClient>(settings);
  }
  void TearDown() override {
    client_.reset();
    network_.reset();
  }

  AuthenticationClient::SignInClientResponse SignInClient(
      const AuthenticationCredentials& credentials,
      unsigned int expires_in = kLimitExpiry, bool do_cancel = false) {
    std::shared_ptr<AuthenticationClient::SignInClientResponse> response;
    unsigned int retry = 0u;
    do {
      if (retry > 0u) {
        OLP_SDK_LOG_WARNING(kLogTag,
                            "Request retry attempted (" << retry << ")");
        std::this_thread::sleep_for(
            std::chrono::seconds(retry * kRetryDelayInSecs));
      }

      std::promise<AuthenticationClient::SignInClientResponse> request;
      auto request_future = request.get_future();

      AuthenticationClient::SignInProperties props;
      props.expires_in = std::chrono::seconds(expires_in);

      auto cancel_token = client_->SignInClient(
          credentials, props,
          [&](const AuthenticationClient::SignInClientResponse& resp) {
            request.set_value(resp);
          });

      if (do_cancel) {
        cancel_token.Cancel();
      }
      response = std::make_shared<AuthenticationClient::SignInClientResponse>(
          request_future.get());
    } while ((!response->IsSuccessful()) && (++retry < kMaxRetryCount) &&
             !do_cancel);

    return *response;
  }

  AuthorizeResponse Authorize(const std::string& access_token,
                              AuthorizeRequest request,
                              bool do_cancel = false) {
    std::shared_ptr<AuthorizeResponse> response;
    auto retry = 0u;
    do {
      if (retry > 0u) {
        OLP_SDK_LOG_WARNING(kLogTag,
                            "Request retry attempted (" << retry << ")");
        std::this_thread::sleep_for(
            std::chrono::seconds(retry * kRetryDelayInSecs));
      }

      std::promise<AuthorizeResponse> resp;
      auto request_future = resp.get_future();

      auto cancel_token = client_->Authorize(
          access_token, std::move(request),
          [&](const AuthorizeResponse& responce) { resp.set_value(responce); });

      if (do_cancel) {
        cancel_token.Cancel();
      }
      response = std::make_shared<AuthorizeResponse>(request_future.get());
    } while ((!response->IsSuccessful()) && (++retry < kMaxRetryCount) &&
             !do_cancel);

    return *response;
  }

  std::string GetErrorId(
      const AuthenticationClient::SignInUserResponse& response) const {
    return response.GetResult().GetErrorResponse().error_id;
  }
};

TEST_F(AuthenticationClientTestAuthorize, AuthorizeAllow) {
  AuthenticationCredentials credentials(id_, secret_);
  auto resp = SignInClient(credentials, kExpiryTime);
  auto res = resp.GetResult();
  EXPECT_TRUE(resp.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::OK, res.GetStatus());

  auto token = res.GetAccessToken();
  auto request = olp::authentication::AuthorizeRequest().WithServiceId(service_id_);
  request.WithAction("getTileCore");
  auto response = Authorize(token, std::move(request));
  auto result = response.GetResult();

  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_FALSE(response.GetResult().GetClientId().empty());
  ASSERT_EQ(result.GetDecision(), olp::authentication::DecisionType::kAllow);
}

TEST_F(AuthenticationClientTestAuthorize, AuthorizeDeny) {
  auto request =
      olp::authentication::AuthorizeRequest().WithServiceId("Wrong_service");
  request.WithAction("getTileCore");

  AuthenticationCredentials credentials(id_, secret_);
  auto resp = SignInClient(credentials, kExpiryTime);
  auto res = resp.GetResult();
  EXPECT_TRUE(resp.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::OK, res.GetStatus());

  auto token = res.GetAccessToken();
  auto response = Authorize(token, std::move(request));

  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_FALSE(response.GetResult().GetClientId().empty());
  ASSERT_EQ(response.GetResult().GetDecision(),
            olp::authentication::DecisionType::kDeny);
}

TEST_F(AuthenticationClientTestAuthorize, AuthorizeWithTwoActions) {
  auto request =
      olp::authentication::AuthorizeRequest()
          .WithServiceId(service_id_)
          .WithAction("getTileCore")
          .WithAction("InvalidAction")
          .WithOperatorType(
              olp::authentication::AuthorizeRequest::DecisionOperatorType::kOr)
          .WithDiagnostics(true);

  AuthenticationCredentials credentials(id_, secret_);
  auto resp = SignInClient(credentials, kExpiryTime);
  auto res = resp.GetResult();
  EXPECT_TRUE(resp.IsSuccessful());
  EXPECT_EQ(olp::http::HttpStatusCode::OK, res.GetStatus());

  auto token = res.GetAccessToken();
  auto response = Authorize(token, std::move(request));

  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_FALSE(response.GetResult().GetClientId().empty());
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
