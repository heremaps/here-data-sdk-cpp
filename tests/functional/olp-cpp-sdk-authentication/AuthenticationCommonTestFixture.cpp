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

#include "AuthenticationCommonTestFixture.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include "TestConstants.h"

using namespace ::olp::authentication;

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
  client_ = std::make_unique<AuthenticationClient>(kHereAccountStagingURL);
  network_ = s_network_;
  task_scheduler_ =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
  client_->SetNetwork(network_);
  client_->SetTaskScheduler(task_scheduler_);
}

void AuthenticationCommonTestFixture::TearDown() {
  client_.reset();
  network_.reset();
}

AuthenticationClient::SignInUserResponse
AuthenticationCommonTestFixture::AcceptTerms(
    const AuthenticationClient::SignInUserResponse& precond_failed_response,
    bool do_cancel) {
  AuthenticationCredentials credentials(GetAppKey(), GetAppSecretKey());

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
      cancel_token.cancel();
    }

    request_future.wait();
    response = std::make_shared<AuthenticationClient::SignInUserResponse>(
        request_future.get());
  } while ((!response->IsSuccessful()) && (++retry < kMaxRetryCount) &&
           !do_cancel);

  return *response;
}

AuthenticationCommonTestFixture::DeleteUserResponse
AuthenticationCommonTestFixture::DeleteUser(
    const std::string& user_bearer_token) {
  std::shared_ptr<DeleteUserResponse> response;
  unsigned int retry = 0u;
  do {
    if (retry > 0u) {
      OLP_SDK_LOG_WARNING(__func__, "Request retry attempted (" << retry
                                                                << ")");
      std::this_thread::sleep_for(
          std::chrono::seconds(retry * kRetryDelayInSecs));
    }

    std::promise<DeleteUserResponse> request;
    auto request_future = request.get_future();
    DeleteHereUser(*network_, olp::http::NetworkSettings(), user_bearer_token,
                   [&request](const DeleteUserResponse& resp) {
                     request.set_value(resp);
                   });
    request_future.wait();
    response = std::make_shared<DeleteUserResponse>(request_future.get());
  } while ((response->status < 0) && (++retry < kMaxRetryCount));

  return *response;
}

AuthenticationClient::SignOutUserResponse
AuthenticationCommonTestFixture::SignOutUser(const std::string& access_token,
                                             bool do_cancel) {
  AuthenticationCredentials credentials(GetAppKey(), GetAppSecretKey());
  std::promise<AuthenticationClient::SignOutUserResponse> request;
  auto request_future = request.get_future();
  auto cancel_token = client_->SignOut(
      credentials, access_token,
      [&](const AuthenticationClient::SignOutUserResponse& response) {
        request.set_value(response);
      });

  if (do_cancel) {
    cancel_token.cancel();
  }

  request_future.wait();
  return request_future.get();
}

std::string AuthenticationCommonTestFixture::GetEmail() const {
  return kTestUserName + "-" + GenerateRandomSequence() + "@example.com";
}

void AuthenticationCommonTestFixture::DeleteHereUser(
    olp::http::Network& network,
    const olp::http::NetworkSettings& network_settings,
    const std::string& user_bearer_token,
    const DeleteHereUserCallback& callback) {
  constexpr auto kAuthorization = "Authorization";
  constexpr auto kContentType = "Content-Type";
  constexpr auto kApplicationJson = "application/json";
  constexpr auto kDeleteUserEndpoint = "/user/me";

  std::string url = kHereAccountStagingURL;
  url.append(kDeleteUserEndpoint);

  olp::http::NetworkRequest request(url);
  request.WithVerb(olp::http::NetworkRequest::HttpVerb::DEL);
  request.WithHeader(kAuthorization, GenerateBearerHeader(user_bearer_token));
  request.WithHeader(kContentType, kApplicationJson);
  request.WithSettings(network_settings);

  std::shared_ptr<std::stringstream> payload =
      std::make_shared<std::stringstream>();
  network.Send(
      request, payload,
      [callback, payload](const olp::http::NetworkResponse& network_response) {
        DeleteUserResponse response;
        response.status = network_response.GetStatus();
        response.error = network_response.GetError();
        callback(response);
      });
}

std::string AuthenticationCommonTestFixture::GenerateBearerHeader(
    const std::string& user_bearer_token) {
  std::string authorization = "Bearer ";
  authorization += user_bearer_token;
  return authorization;
}

std::string AuthenticationCommonTestFixture::GenerateRandomSequence() const {
  static boost::uuids::random_generator gen;
  return boost::uuids::to_string(gen());
}
