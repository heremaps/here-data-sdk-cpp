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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <future>
#include <memory>

#include "ArcGisTestUtils.h"
#include "AuthenticationUtils.h"
#include "CommonTestUtils.h"
#include "FacebookTestUtils.h"
#include "GoogleTestUtils.h"
#include "OfflineResponses.h"
#include "SignInUserResultImpl.h"
#include "olp/authentication/AuthenticationClient.h"
#include "olp/core/client/ErrorCode.h"
#include "olp/core/client/OlpClientSettingsFactory.h"
#include "olp/core/http/Network.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"
#include "olp/core/thread/TaskScheduler.h"
#include "testutils/CustomParameters.hpp"

using namespace olp::authentication;

namespace {
constexpr unsigned int EXPIRY_TIME = 3600;
constexpr unsigned int MAX_EXPIRY = EXPIRY_TIME + 30;
constexpr unsigned int MIN_EXPIRY = EXPIRY_TIME - 10;

constexpr unsigned int CUSTOM_EXPIRY_TIME = 6000;
constexpr unsigned int MAX_CUSTOM_EXPIRY = CUSTOM_EXPIRY_TIME + 30;
constexpr unsigned int MIN_CUSTOM_EXPIRY = CUSTOM_EXPIRY_TIME - 10;

constexpr unsigned int EXTENDED_EXPIRY_TIME = 2 * EXPIRY_TIME;
constexpr unsigned int MAX_EXTENDED_EXPIRY = EXTENDED_EXPIRY_TIME + 30;
constexpr unsigned int MIN_EXTENDED_EXPIRY = EXTENDED_EXPIRY_TIME - 10;

constexpr unsigned int LIMIT_EXPIRY = 86400;
constexpr unsigned int MAX_LIMIT_EXPIRY = LIMIT_EXPIRY + 30;
constexpr unsigned int MIN_LIMIT_EXPIRY = LIMIT_EXPIRY - 10;
}  // namespace

class AuthenticationBaseTest : public ::testing::Test {
 public:
  void SetUp() override {
    client_ = std::make_unique<AuthenticationClient>(HERE_ACCOUNT_STAGING_URL);
    utils_ = std::make_unique<AuthenticationUtils>();

    id_ = CustomParameters::getArgument("service_id");
    secret_ = CustomParameters::getArgument("service_secret");
  }

  void TearDown() override {
    client_.reset();
    utils_.reset();

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  AuthenticationClient::SignUpResponse SignUpUser(
      const std::string& email, const std::string& password = "password123",
      bool do_cancel = false) {
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

  AuthenticationClient::SignOutUserResponse SignOutUser(
      const std::string& access_token, bool do_cancel = false) {
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

 protected:
  std::unique_ptr<AuthenticationClient> client_;
  std::unique_ptr<AuthenticationUtils> utils_;
  std::shared_ptr<olp::http::Network> network_;
  std::shared_ptr<olp::thread::TaskScheduler> task_scheduler_;
  std::string id_;
  std::string secret_;
};
