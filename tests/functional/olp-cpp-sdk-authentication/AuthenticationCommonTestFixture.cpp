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
#include <testutils/CustomParameters.hpp>

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
  utils_ = std::make_unique<AuthenticationUtils>();
  network_ = s_network_;
  task_scheduler_ =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();

  client_ = std::make_unique<AuthenticationClient>(kHereAccountStagingURL);
  client_->SetNetwork(network_);
  client_->SetTaskScheduler(task_scheduler_);

  id_ = CustomParameters::getArgument("service_id");
  secret_ = CustomParameters::getArgument("service_secret");
}

void AuthenticationCommonTestFixture::TearDown() {
  client_.reset();
  network_.reset();
  utils_.reset();

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
      cancel_token.cancel();
    }

    request_future.wait();
    response = std::make_shared<AuthenticationClient::SignInUserResponse>(
        request_future.get());
  } while ((!response->IsSuccessful()) && (++retry < kMaxRetryCount) &&
           !do_cancel);

  return *response;
}

AuthenticationUtils::DeleteUserResponse
AuthenticationCommonTestFixture::DeleteUser(
    const std::string& user_bearer_token) {
  std::shared_ptr<AuthenticationUtils::DeleteUserResponse> response;
  unsigned int retry = 0u;
  do {
    if (retry > 0u) {
      OLP_SDK_LOG_WARNING(__func__, "Request retry attempted (" << retry
                                                                << ")");
      std::this_thread::sleep_for(
          std::chrono::seconds(retry * kRetryDelayInSecs));
    }

    std::promise<AuthenticationUtils::DeleteUserResponse> request;
    auto request_future = request.get_future();
    utils_->DeleteHereUser(
        *network_, olp::http::NetworkSettings(), user_bearer_token,
        [&request](const AuthenticationUtils::DeleteUserResponse& resp) {
          request.set_value(resp);
        });
    request_future.wait();
    response = std::make_shared<AuthenticationUtils::DeleteUserResponse>(
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
    cancel_token.cancel();
  }

  request_future.wait();
  return request_future.get();
}

std::string AuthenticationCommonTestFixture::GetEmail() const {
  return kTestUserName + "-" + GenerateRandomSequence() + "@example.com";
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

AuthenticationClient::SignInClientResponse
AuthenticationCommonTestFixture::SignInClient(
    const AuthenticationCredentials& credentials, std::time_t& now,
    unsigned int expires_in, bool do_cancel) {
  std::shared_ptr<AuthenticationClient::SignInClientResponse> response;
  unsigned int retry = 0u;
  do {
    if (retry > 0u) {
      OLP_SDK_LOG_WARNING(__func__, "Request retry attempted (" << retry
                                                                << ")");
      std::this_thread::sleep_for(
          std::chrono::seconds(retry * kRetryDelayInSecs));
    }

    std::promise<AuthenticationClient::SignInClientResponse> request;
    auto request_future = request.get_future();

    now = std::time(nullptr);
    auto cancel_token = client_->SignInClient(
        credentials,
        [&](const AuthenticationClient::SignInClientResponse& resp) {
          request.set_value(resp);
        },
        std::chrono::seconds(expires_in));

    if (do_cancel) {
      cancel_token.cancel();
    }
    request_future.wait();
    response = std::make_shared<AuthenticationClient::SignInClientResponse>(
        request_future.get());
  } while ((!response->IsSuccessful()) && (++retry < kMaxRetryCount) &&
           !do_cancel);

  return *response;
}

AuthenticationClient::SignInUserResponse
AuthenticationCommonTestFixture::SignInUser(const std::string& email,
                                            bool do_cancel) {
  AuthenticationCredentials credentials(id_, secret_);
  AuthenticationClient::UserProperties properties;
  properties.email = email;
  properties.password = "password123";

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
    auto cancel_token = client_->SignInHereUser(
        credentials, properties,
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

AuthenticationClient::SignInUserResponse
AuthenticationCommonTestFixture::SignInRefesh(const std::string& access_token,
                                              const std::string& refresh_token,
                                              bool do_cancel) {
  AuthenticationCredentials credentials(id_, secret_);
  AuthenticationClient::RefreshProperties properties;
  properties.access_token = access_token;
  properties.refresh_token = refresh_token;

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
    auto cancel_token = client_->SignInRefresh(
        credentials, properties,
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

AuthenticationClient::SignUpResponse
AuthenticationCommonTestFixture::SignUpUser(const std::string& email,
                                            const std::string& password,
                                            bool do_cancel) {
  AuthenticationCredentials credentials(id_, secret_);
  std::promise<AuthenticationClient::SignUpResponse> request;
  auto request_future = request.get_future();
  AuthenticationClient::SignUpProperties properties;
  properties.email = email;
  properties.password = password;
  properties.date_of_birth = "31/01/1980";
  properties.first_name = "AUTH_TESTER";
  properties.last_name = "HEREOS";
  properties.country_code = "USA";
  properties.language = "en";
  properties.phone_number = "+1234567890";
  auto cancel_token = client_->SignUpHereUser(
      credentials, properties,
      [&](const AuthenticationClient::SignUpResponse& response) {
        request.set_value(response);
      });

  if (do_cancel) {
    cancel_token.cancel();
  }

  request_future.wait();
  return request_future.get();
}
