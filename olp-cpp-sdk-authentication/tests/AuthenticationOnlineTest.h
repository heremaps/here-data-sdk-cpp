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

class AuthenticationOnlineTest : public AuthenticationBaseTest {
 public:
  static void SetUpTestSuite() {
    s_network_ = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler(1);
  }

  static void TearDownTestSuite() { s_network_.reset(); }

  void SetUp() override {
    AuthenticationBaseTest::SetUp();
    network_ = s_network_;
    task_scheduler_ =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
    client_->SetNetwork(network_);
    client_->SetTaskScheduler(task_scheduler_);
  }

  AuthenticationClient::SignInClientResponse SignInClient(
      const AuthenticationCredentials& credentials, std::time_t& now,
      unsigned int expires_in = LIMIT_EXPIRY, bool do_cancel = false) {
    std::shared_ptr<AuthenticationClient::SignInClientResponse> response;
    unsigned int retry = 0u;
    do {
      if (retry > 0u) {
        EDGE_SDK_LOG_WARNING(__func__,
                             "Request retry attempted (" << retry << ")");
        std::this_thread::sleep_for(
            std::chrono::seconds(retry * RETRY_DELAY_SECS));
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
    } while ((!response->IsSuccessful()) && (++retry < MAX_RETRY_COUNT) &&
             !do_cancel);

    return *response;
  }

  AuthenticationClient::SignInUserResponse SignInUser(const std::string& email,
                                                      bool do_cancel = false) {
    AuthenticationCredentials credentials(id_, secret_);
    AuthenticationClient::UserProperties properties;
    properties.email = email;
    properties.password = "password123";

    std::shared_ptr<AuthenticationClient::SignInUserResponse> response;
    unsigned int retry = 0u;
    do {
      if (retry > 0u) {
        EDGE_SDK_LOG_WARNING(__func__,
                             "Request retry attempted (" << retry << ")");
        std::this_thread::sleep_for(
            std::chrono::seconds(retry * RETRY_DELAY_SECS));
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
    } while ((!response->IsSuccessful()) && (++retry < MAX_RETRY_COUNT) &&
             !do_cancel);

    return *response;
  }

  AuthenticationClient::SignInUserResponse SignInRefesh(
      const std::string& access_token, const std::string& refresh_token,
      bool do_cancel = false) {
    AuthenticationCredentials credentials(id_, secret_);
    AuthenticationClient::RefreshProperties properties;
    properties.access_token = access_token;
    properties.refresh_token = refresh_token;

    std::shared_ptr<AuthenticationClient::SignInUserResponse> response;
    unsigned int retry = 0u;
    do {
      if (retry > 0u) {
        EDGE_SDK_LOG_WARNING(__func__,
                             "Request retry attempted (" << retry << ")");
        std::this_thread::sleep_for(
            std::chrono::seconds(retry * RETRY_DELAY_SECS));
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
    } while ((!response->IsSuccessful()) && (++retry < MAX_RETRY_COUNT) &&
             !do_cancel);

    return *response;
  }

  AuthenticationClient::SignInUserResponse AcceptTerms(
      const AuthenticationClient::SignInUserResponse& precond_failed_response,
      bool do_cancel = false) {
    AuthenticationCredentials credentials(id_, secret_);

    std::shared_ptr<AuthenticationClient::SignInUserResponse> response;
    unsigned int retry = 0u;
    do {
      if (retry > 0u) {
        EDGE_SDK_LOG_WARNING(__func__,
                             "Request retry attempted (" << retry << ")");
        std::this_thread::sleep_for(
            std::chrono::seconds(retry * RETRY_DELAY_SECS));
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
    } while ((!response->IsSuccessful()) && (++retry < MAX_RETRY_COUNT) &&
             !do_cancel);

    return *response;
  }

  AuthenticationUtils::DeleteUserResponse DeleteUser(
      const std::string& user_bearer_token) {
    std::shared_ptr<AuthenticationUtils::DeleteUserResponse> response;
    unsigned int retry = 0u;
    do {
      if (retry > 0u) {
        EDGE_SDK_LOG_WARNING(__func__,
                             "Request retry attempted (" << retry << ")");
        std::this_thread::sleep_for(
            std::chrono::seconds(retry * RETRY_DELAY_SECS));
      }

      std::promise<AuthenticationUtils::DeleteUserResponse> request;
      auto request_future = request.get_future();
      utils_->deleteHereUser(
          *network_, olp::http::NetworkSettings(), user_bearer_token,
          [&request](const AuthenticationUtils::DeleteUserResponse& resp) {
            request.set_value(resp);
          });
      request_future.wait();
      response = std::make_shared<AuthenticationUtils::DeleteUserResponse>(
          request_future.get());
    } while ((response->status < 0) && (++retry < MAX_RETRY_COUNT));

    return *response;
  }

  std::string GenerateRandomSequence() const {
    static boost::uuids::random_generator gen;
    return boost::uuids::to_string(gen());
  }

  std::string GetEmail() const {
    return TEST_USER_NAME + "-" + GenerateRandomSequence() + "@example.com";
  }

 protected:
  static std::shared_ptr<olp::http::Network> s_network_;
};
