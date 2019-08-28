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
#include "MockTestConfig.h"
#include "OfflineResponses.h"
#include "SignInUserResultImpl.h"
#include "olp/authentication/AuthenticationClient.h"
#include "olp/core/client/ErrorCode.h"
#include "olp/core/logging/Log.h"
#include "olp/core/network/Network.h"
#include "olp/core/porting/make_unique.h"
#include "testutils/CustomParameters.hpp"

using namespace olp::authentication;
using namespace olp::network;

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
  std::string id_;
  std::string secret_;
};

class AuthenticationOfflineTest : public AuthenticationBaseTest {
 public:
  void SetUp() override {
    AuthenticationBaseTest::SetUp();
    mockNetworkTestApp.setUp();
  }

  void TearDown() override {
    mockNetworkTestApp.tearDown();
    AuthenticationBaseTest::TearDown();
  }

  std::shared_ptr<test::MockNetworkRequestBuilder> ExpectationBuilder() {
    return std::make_shared<test::MockNetworkRequestBuilder>(
        *mockNetworkTestApp.m_protocolMock);
  }

  void ExecuteSigninRequest(int http, int http_result,
                            const std::string& error_message,
                            const std::string& data = "", int error_code = 0) {
    AuthenticationCredentials credentials(id_, secret_);
    std::promise<AuthenticationClient::SignInClientResponse> request;
    auto request_future = request.get_future();

    auto expectation = ExpectationBuilder();
    expectation->forUrl(signin_request)
        .withResponseData(std::vector<char>(data.begin(), data.end()))
        .withReturnCode(http)
        .withErrorString("")
        .completeSynchronously()
        .buildExpectation();

    client_->SignInClient(
        credentials,
        [&](const AuthenticationClient::SignInClientResponse& response) {
          request.set_value(response);
        });
    request_future.wait();
    AuthenticationClient::SignInClientResponse response = request_future.get();
    if (response.IsSuccessful()) {
      EXPECT_EQ(http_result, response.GetResult().GetStatus());
      EXPECT_EQ(error_message, response.GetResult().GetErrorResponse().message);
      if (error_code != 0) {
        EXPECT_EQ(error_code, response.GetResult().GetErrorResponse().code);
      }
    }
  }

 protected:
  test::MockNetworkTestApp mockNetworkTestApp;
};

class AuthenticationOnlineTest : public AuthenticationBaseTest {
 public:
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
          user_bearer_token,
          [&request](const AuthenticationUtils::DeleteUserResponse& resp) {
            request.set_value(resp);
          });
      request_future.wait();
      response = std::make_shared<AuthenticationUtils::DeleteUserResponse>(
          request_future.get());
    } while ((response->status < 0) && (++retry < MAX_RETRY_COUNT));

    return *response;
  }

  std::string GenerateRandomSequence() {
    static boost::uuids::random_generator gen;
    return boost::uuids::to_string(gen());
  }

  std::string GetEmail() {
    return TEST_USER_NAME + "-" + GenerateRandomSequence() + "@example.com";
  }
};

class FacebookAuthenticationOnlineTest : public AuthenticationOnlineTest {
 public:
  void SetUp() override {
    AuthenticationOnlineTest::SetUp();
    Facebook = std::make_unique<FacebookTestUtils>();
    ASSERT_TRUE(Facebook->createFacebookTestUser(testUser, "email"));
    id_ = TEST_APP_KEY_ID;
    secret_ = TEST_APP_KEY_SECRET;
  }

  AuthenticationClient::SignInUserResponse SignInFacebook(
      std::string token = "") {
    AuthenticationCredentials credentials(id_, secret_);
    std::promise<AuthenticationClient::SignInUserResponse> request;
    auto request_future = request.get_future();
    AuthenticationClient::FederatedProperties properties;
    properties.access_token = token.empty() ? testUser.access_token : token;
    properties.country_code = "usa";
    properties.language = "en";
    properties.email = TEST_USER_NAME + "@example.com";
    client_->SignInFacebook(
        credentials, properties,
        [&](const AuthenticationClient::SignInUserResponse& response) {
          request.set_value(response);
        });
    request_future.wait();
    return request_future.get();
  }

  void deleteFacebookTestUser(const std::string& id) {
    for (int retry = 0; retry < 3; ++retry) {
      if (Facebook->deleteFacebookTestUser(id)) {
        return;
      }

      std::this_thread::sleep_for(std::chrono::seconds(retry));
    }
  }

  void TearDown() override {
    deleteFacebookTestUser(testUser.id);
    AuthenticationOnlineTest::TearDown();
    Facebook.reset();
  }

 private:
  std::unique_ptr<FacebookTestUtils> Facebook;
  FacebookTestUtils::FacebookUser testUser;
};

class GoogleAuthenticationOnlineTest : public AuthenticationOnlineTest {
 public:
  void SetUp() override {
    AuthenticationOnlineTest::SetUp();
    google = std::make_unique<GoogleTestUtils>();
    ASSERT_TRUE(google->getAccessToken(testUser));
    id_ = TEST_APP_KEY_ID;
    secret_ = TEST_APP_KEY_SECRET;
  }

  AuthenticationClient::SignInUserResponse SignInGoogleUser(
      const std::string& email, const std::string& access_token) {
    AuthenticationCredentials credentials(id_, secret_);
    std::promise<AuthenticationClient::SignInUserResponse> request;
    auto request_future = request.get_future();
    AuthenticationClient::FederatedProperties properties;
    properties.access_token = access_token;
    properties.country_code = "USA";
    properties.language = "en";
    properties.email = email;
    client_->SignInGoogle(
        credentials, properties,
        [&](const AuthenticationClient::SignInUserResponse& response) {
          request.set_value(response);
        });
    request_future.wait();
    return request_future.get();
  }

  void TearDown() override {
    AuthenticationOnlineTest::TearDown();
    google.reset();
  }

 public:
  GoogleTestUtils::GoogleUser testUser;

 private:
  std::unique_ptr<GoogleTestUtils> google;
};

class ArcGisAuthenticationOnlineTest : public AuthenticationOnlineTest {
 public:
  void SetUp() override {
    AuthenticationOnlineTest::SetUp();
    arcGis = std::make_unique<ArcGisTestUtils>();
    ASSERT_TRUE(arcGis->getAccessToken(testUser));
    id_ = TEST_APP_KEY_ID;
    secret_ = TEST_APP_KEY_SECRET;
  }

  AuthenticationClient::SignInUserResponse SignInArcGis(
      const std::string& email, const std::string& token = "") {
    AuthenticationCredentials credentials(id_, secret_);
    std::promise<AuthenticationClient::SignInUserResponse> request;
    auto request_future = request.get_future();
    AuthenticationClient::FederatedProperties properties;
    properties.access_token = token.empty() ? testUser.access_token : token;
    properties.country_code = "usa";
    properties.language = "en";
    properties.email = email;
    client_->SignInArcGis(
        credentials, properties,
        [&](const AuthenticationClient::SignInUserResponse& response) {
          request.set_value(response);
        });
    request_future.wait();
    return request_future.get();
  }

  void TearDown() override {
    AuthenticationOnlineTest::TearDown();
    arcGis.reset();
  }

 private:
  std::unique_ptr<ArcGisTestUtils> arcGis;
  ArcGisTestUtils::ArcGisUser testUser;
};
