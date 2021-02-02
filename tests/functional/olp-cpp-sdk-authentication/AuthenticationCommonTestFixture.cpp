/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include "AuthenticationCommonTestFixture.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include "AuthenticationTestUtils.h"
#include "TestConstants.h"

std::shared_ptr<olp::http::Network> AuthenticationCommonTestFixture::s_network_;

void AuthenticationCommonTestFixture::SetUpTestSuite() {
  s_network_ =
      olp::client::OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler(
          1);
}

void AuthenticationCommonTestFixture::TearDownTestSuite() {
  s_network_.reset();
}

void AuthenticationCommonTestFixture::SetUp() {
  network_ = s_network_;
  task_scheduler_ =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();

  AuthenticationSettings settings;
  settings.network_request_handler = network_;
  settings.task_scheduler = task_scheduler_;
  settings.token_endpoint_url = kHereAccountStagingURL;

  client_ = std::make_unique<AuthenticationClient>(settings);
}

void AuthenticationCommonTestFixture::TearDown() {
  client_.reset();
  network_.reset();

  // TODO - what is the reason behind this sleep?
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

AuthenticationClient::SignInUserResponse
AuthenticationCommonTestFixture::AcceptTerms(
    const AuthenticationClient::SignInUserResponse& precond_failed_response,
    bool do_cancel) {
  AuthenticationCredentials credentials(id_, secret_);

  std::shared_ptr<AuthenticationClient::SignInUserResponse> response;
  unsigned int retry = 0u;
  do {
    if (retry > 0u) {
      OLP_SDK_LOG_WARNING(__func__, "Request retry attempted (" << retry
                                                                << ")");
      std::this_thread::sleep_for(
          std::chrono::seconds(retry * kRetryDelayInSecs));
    }

    std::promise<AuthenticationClient::SignInUserResponse> request;
    auto request_future = request.get_future();
    auto cancel_token = client_->AcceptTerms(
        credentials,
        precond_failed_response.GetResult().GetTermAcceptanceToken(),
        [&request](const AuthenticationClient::SignInUserResponse& resp) {
          request.set_value(resp);
        });

    if (do_cancel) {
      cancel_token.Cancel();
    }

    response = std::make_shared<AuthenticationClient::SignInUserResponse>(
        request_future.get());
  } while ((!response->IsSuccessful()) && (++retry < kMaxRetryCount) &&
           !do_cancel);

  return *response;
}

AuthenticationTestUtils::DeleteUserResponse
AuthenticationCommonTestFixture::DeleteUser(
    const std::string& user_bearer_token) {
  std::shared_ptr<AuthenticationTestUtils::DeleteUserResponse> response;
  unsigned int retry = 0u;
  do {
    if (retry > 0u) {
      OLP_SDK_LOG_WARNING(__func__, "Request retry attempted (" << retry
                                                                << ")");
      std::this_thread::sleep_for(
          std::chrono::seconds(retry * kRetryDelayInSecs));
    }

    std::promise<AuthenticationTestUtils::DeleteUserResponse> request;
    auto request_future = request.get_future();
    AuthenticationTestUtils::DeleteHereUser(
        *network_, olp::http::NetworkSettings(), user_bearer_token,
        [&request](const AuthenticationTestUtils::DeleteUserResponse& resp) {
          request.set_value(resp);
        });
    response = std::make_shared<AuthenticationTestUtils::DeleteUserResponse>(
        request_future.get());
  } while ((response->status < 0) && (++retry < kMaxRetryCount));

  return *response;
}

AuthenticationClient::SignOutUserResponse
AuthenticationCommonTestFixture::SignOutUser(const std::string& access_token,
                                             bool do_cancel) {
  AuthenticationCredentials credentials(id_, secret_);
  std::promise<AuthenticationClient::SignOutUserResponse> request;
  auto request_future = request.get_future();
  auto cancel_token = client_->SignOut(
      credentials, access_token,
      [&](const AuthenticationClient::SignOutUserResponse& response) {
        request.set_value(response);
      });

  if (do_cancel) {
    cancel_token.Cancel();
  }

  return request_future.get();
}

std::string AuthenticationCommonTestFixture::GetEmail() const {
  return kTestUserName + "-" + GenerateRandomSequence() + "@example.com";
}

std::string AuthenticationCommonTestFixture::GenerateRandomSequence() const {
  static boost::uuids::random_generator gen;
  return boost::uuids::to_string(gen());
}
