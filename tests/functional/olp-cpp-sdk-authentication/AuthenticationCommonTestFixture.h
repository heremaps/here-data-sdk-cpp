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

#pragma once

#include <memory>

#include <gtest/gtest.h>
#include <olp/core/http/Network.h>
#include <olp/authentication/AuthenticationClient.h>

class AuthenticationCommonTestFixture : public ::testing::Test {
 protected:
  struct DeleteUserResponse {
    int status;
    std::string error;
  };

  static void SetUpTestSuite();

  static void TearDownTestSuite();

  void SetUp() override;

  void TearDown() override;

  olp::authentication::AuthenticationClient::SignInUserResponse AcceptTerms(
      const olp::authentication::AuthenticationClient::SignInUserResponse&
          precond_failed_response,
      bool do_cancel = false);

  DeleteUserResponse DeleteUser(const std::string& user_bearer_token);

  olp::authentication::AuthenticationClient::SignOutUserResponse SignOutUser(
      const std::string& access_token, bool do_cancel = false);

  std::string GetEmail() const;

  virtual std::string GetAppKey() const = 0;

  virtual std::string GetAppSecretKey() const = 0;

 protected:
  std::shared_ptr<olp::authentication::AuthenticationClient> client_;
  std::shared_ptr<olp::http::Network> network_;
  std::shared_ptr<olp::thread::TaskScheduler> task_scheduler_;

  static std::shared_ptr<olp::http::Network> s_network_;

 private:
  using DeleteHereUserCallback =
      std::function<void(const DeleteUserResponse& response)>;

  void DeleteHereUser(olp::http::Network& network,
                      const olp::http::NetworkSettings& network_settings,
                      const std::string& user_bearer_token,
                      const DeleteHereUserCallback& callback);

  std::string GenerateBearerHeader(const std::string& user_bearer_token);

  std::string GenerateRandomSequence() const;
};
