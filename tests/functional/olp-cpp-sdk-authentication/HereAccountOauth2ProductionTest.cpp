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

#include <thread>

#include <gmock/gmock.h>
#include <olp/authentication/AuthenticationClient.h>
#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/NetworkProxySettings.h>
#include <olp/core/porting/warning_disable.h>
#include <testutils/CustomParameters.hpp>
#include "AutoRefreshingToken.h"
#include "TokenRequest.h"
#include "Utils.h"

namespace authentication = ::olp::authentication;
namespace http = olp::http;

//  OLPEDGE-1797
PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

namespace {
constexpr auto kTestMaxExecutionTime = std::chrono::seconds(30);

const auto IsUnauthorizedMessage = testing::AllOf(
    testing::HasSubstr("errorId"), testing::HasSubstr("httpStatus"),
    testing::HasSubstr("errorCode"), testing::HasSubstr("message"),
    testing::HasSubstr("401300"));

authentication::TokenResponse GetTokenFromSyncRequest(
    const authentication::AutoRefreshingToken& auto_token,
    const std::chrono::seconds minimum_validity =
        authentication::kDefaultMinimumValiditySeconds) {
  return auto_token.GetToken(minimum_validity);
}

authentication::TokenResponse GetTokenFromAsyncRequest(
    const authentication::AutoRefreshingToken& auto_token,
    const std::chrono::seconds minimum_validity =
        authentication::kDefaultMinimumValiditySeconds) {
  std::promise<authentication::TokenResponse> promise;
  auto future = promise.get_future();
  auto_token.GetToken(
      [&promise](authentication::TokenResponse token_response) {
        promise.set_value(token_response);
      },
      minimum_validity);
  return future.get();
}

void TestAutoRefreshingTokenValidRequest(
    authentication::TokenEndpoint& token_endpoint,
    std::function<authentication::TokenResponse(
        const authentication::AutoRefreshingToken& auto_token)>
        func) {
  auto token_response =
      func(authentication::AutoRefreshingToken(token_endpoint, {}));
  ASSERT_TRUE(token_response);
  EXPECT_GT(token_response.GetResult().GetAccessToken().length(), 42u);
  EXPECT_GT(token_response.GetResult().GetExpiryTime(), time(nullptr));
}

void TestAutoRefreshingTokenInvalidRequest(
    const std::shared_ptr<olp::http::Network>& network,
    std::function<authentication::TokenResponse(
        const authentication::AutoRefreshingToken& auto_token)>
        func) {
  authentication::Settings settings({"BAD", "BAD"});
  settings.task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
  settings.network_request_handler = network;
  auto bad_token_endpoint = authentication::TokenEndpoint(settings);
  auto token_response =
      func(authentication::AutoRefreshingToken(bad_token_endpoint, {}));
  ASSERT_FALSE(token_response);
  EXPECT_EQ(token_response.GetError().GetHttpStatusCode(),
            olp::http::HttpStatusCode::UNAUTHORIZED);
  EXPECT_THAT(token_response.GetError().GetMessage(), IsUnauthorizedMessage);
}

void TestAutoRefreshingTokenReuseToken(
    authentication::TokenEndpoint& token_endpoint,
    std::function<authentication::TokenResponse(
        const authentication::AutoRefreshingToken& auto_token)>
        func) {
  auto auto_token = authentication::AutoRefreshingToken(token_endpoint, {});
  auto token_response_one = func(auto_token);
  auto token_response_two = func(auto_token);
  EXPECT_EQ(token_response_one.GetResult().GetAccessToken(),
            token_response_two.GetResult().GetAccessToken());
  EXPECT_EQ(token_response_one.GetResult().GetExpiryTime(),
            token_response_two.GetResult().GetExpiryTime());
}

void TestAutoRefreshingTokenForceRefresh(
    authentication::TokenEndpoint& token_endpoint,
    std::function<authentication::TokenResponse(
        const authentication::AutoRefreshingToken& auto_token,
        const std::chrono::seconds minimum_validity)>
        func) {
  auto auto_token = authentication::AutoRefreshingToken(token_endpoint, {});
  auto token_response_one = func(auto_token, std::chrono::minutes(5));
  auto token_response_two = func(auto_token, authentication::kForceRefresh);

  EXPECT_NE(token_response_one.GetResult().GetAccessToken(),
            token_response_two.GetResult().GetAccessToken());
}

void TestAutoRefreshingTokenExpiresInRefresh(
    authentication::TokenEndpoint& token_endpoint,
    std::function<authentication::TokenResponse(
        const authentication::AutoRefreshingToken& auto_token)>
        func) {
  auto auto_token = authentication::AutoRefreshingToken(
      token_endpoint, authentication::TokenRequest{std::chrono::seconds(302)});
  auto token_response_one = func(auto_token);
  std::this_thread::sleep_for(std::chrono::seconds(4));
  auto token_response_two = func(auto_token);

  EXPECT_NE(token_response_one.GetResult().GetAccessToken(),
            token_response_two.GetResult().GetAccessToken());
  EXPECT_NE(token_response_one.GetResult().GetExpiryTime(),
            token_response_two.GetResult().GetExpiryTime());
}

void TestAutoRefreshingTokenExpiresDoNotRefresh(
    authentication::TokenEndpoint& token_endpoint,
    std::function<authentication::TokenResponse(
        const authentication::AutoRefreshingToken& auto_token)>
        func) {
  auto auto_token = authentication::AutoRefreshingToken(
      token_endpoint, authentication::TokenRequest{std::chrono::seconds(305)});
  auto token_response_one = func(auto_token);
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto token_response_two = func(auto_token);

  EXPECT_EQ(token_response_one.GetResult().GetAccessToken(),
            token_response_two.GetResult().GetAccessToken());
  EXPECT_EQ(token_response_one.GetResult().GetExpiryTime(),
            token_response_two.GetResult().GetExpiryTime());
}

void TestAutoRefreshingTokenExpiresDoRefresh(
    authentication::TokenEndpoint& token_endpoint,
    std::function<authentication::TokenResponse(
        const authentication::AutoRefreshingToken& auto_token,
        const std::chrono::seconds minimum_validity)>
        func) {
  auto auto_token = authentication::AutoRefreshingToken(
      token_endpoint,
      authentication::TokenRequest{std::chrono::seconds(1)});  // 1 second
  auto token_response_one =
      func(auto_token, std::chrono::seconds(1));  // 1 sec validity window,
  // shot enough to trigger
  // a refresh
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto token_response_two =
      func(auto_token, std::chrono::seconds(1));  // 1 sec validity window,
  // shot enough to trigger
  // a refresh

  EXPECT_NE(token_response_one.GetResult().GetAccessToken(),
            token_response_two.GetResult().GetAccessToken());
  EXPECT_NE(token_response_one.GetResult().GetExpiryTime(),
            token_response_two.GetResult().GetExpiryTime());
}

void TestAutoRefreshingTokenExpiresInAnHour(
    authentication::TokenEndpoint& token_endpoint,
    std::function<authentication::TokenResponse(
        const authentication::AutoRefreshingToken& auto_token,
        const std::chrono::seconds minimum_validity)>
        func) {
  auto auto_token = authentication::AutoRefreshingToken(
      token_endpoint, authentication::TokenRequest{std::chrono::hours(1)});
  auto token_response_one = func(auto_token, std::chrono::seconds(1));
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto token_response_two = func(auto_token, std::chrono::seconds(1));

  EXPECT_EQ(token_response_one.GetResult().GetAccessToken(),
            token_response_two.GetResult().GetAccessToken());
  EXPECT_EQ(token_response_one.GetResult().GetExpiryTime(),
            token_response_two.GetResult().GetExpiryTime());
}

void TestAutoRefreshingTokenExpiresInASecond(
    authentication::TokenEndpoint& token_endpoint,
    std::function<authentication::TokenResponse(
        const authentication::AutoRefreshingToken& auto_token,
        const std::chrono::seconds minimum_validity)>
        func) {
  auto auto_token = authentication::AutoRefreshingToken(
      token_endpoint, authentication::TokenRequest{std::chrono::seconds(1)});
  auto token_response_one = func(auto_token, std::chrono::seconds(1));
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto token_response_two = func(auto_token, std::chrono::seconds(1));

  EXPECT_NE(token_response_one.GetResult().GetAccessToken(),
            token_response_two.GetResult().GetAccessToken());
  EXPECT_NE(token_response_one.GetResult().GetExpiryTime(),
            token_response_two.GetResult().GetExpiryTime());
}

void TestAutoRefreshingTokenMultiThread(
    authentication::TokenEndpoint& token_endpoint,
    std::function<authentication::TokenResponse(
        const authentication::AutoRefreshingToken& auto_token)>
        func) {
  auto auto_token = authentication::AutoRefreshingToken(token_endpoint, {});

  std::thread threads[5];
  auto token_responses = std::vector<authentication::TokenResponse>();
  std::mutex token_responses_mutex;

  for (int i = 0; i < 5; i++) {
    threads[i] = std::thread([&]() {
      std::lock_guard<std::mutex> guard(token_responses_mutex);
      token_responses.emplace_back(func(auto_token));
    });
  }

  for (int i = 0; i < 5; i++) {
    threads[i].join();
  }

  ASSERT_EQ(token_responses.size(), 5u);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(token_responses[i].GetResult().GetAccessToken(),
              token_responses[i + 1].GetResult().GetAccessToken());
    EXPECT_EQ(token_responses[i].GetResult().GetExpiryTime(),
              token_responses[i + 1].GetResult().GetExpiryTime());
  }
}

class HereAccountOuauth2ProductionTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    s_network_ = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();
  }

  static void TearDownTestSuite() { s_network_.reset(); }

  HereAccountOuauth2ProductionTest()
      : settings_([]() -> authentication::Settings {
          const auto app_id = CustomParameters::getArgument(
              "integration_production_service_id");
          const auto secret = CustomParameters::getArgument(
              "integration_production_service_secret");
          authentication::Settings settings({app_id, secret});
          settings.task_scheduler = olp::client::OlpClientSettingsFactory::
              CreateDefaultTaskScheduler();
          settings.network_request_handler = s_network_;
          return settings;
        }()),
        token_endpoint_(authentication::TokenEndpoint(settings_)) {}

  authentication::Settings settings_;
  authentication::TokenEndpoint token_endpoint_;

 protected:
  static std::shared_ptr<olp::http::Network> s_network_;
};

std::shared_ptr<olp::http::Network>
    HereAccountOuauth2ProductionTest::s_network_;

TEST_F(HereAccountOuauth2ProductionTest, TokenProviderValidCredentialsValid) {
  authentication::TokenProviderDefault prov{settings_};
  ASSERT_TRUE(prov);

  olp::client::CancellationContext context;
  auto token_response = prov(context);
  ASSERT_TRUE(token_response);
  EXPECT_FALSE(token_response.GetResult().GetAccessToken().empty());

  ASSERT_TRUE(prov);

  token_response = prov(context);
  ASSERT_TRUE(token_response);
  EXPECT_FALSE(token_response.GetResult().GetAccessToken().empty());
}

TEST_F(HereAccountOuauth2ProductionTest, TokenProviderValidCredentialsInvalid) {
  auto token_provider_test = [this](std::string key, std::string secret) {
    auto settings = settings_;
    settings.credentials = {key, secret};
    authentication::TokenProviderDefault prov{settings};
    ASSERT_FALSE(prov);

    olp::client::CancellationContext context;
    auto token_response = prov(context);
    ASSERT_FALSE(token_response);
    EXPECT_EQ(token_response.GetError().GetHttpStatusCode(),
              olp::http::HttpStatusCode::UNAUTHORIZED);
    EXPECT_THAT(token_response.GetError().GetMessage(), IsUnauthorizedMessage);
  };

  token_provider_test("BAD", CustomParameters::getArgument(
                                 "integration_production_service_secret"));
  token_provider_test(
      CustomParameters::getArgument("integration_production_service_id"),
      "BAD");
  token_provider_test("BAD", "BAD");
}

TEST_F(HereAccountOuauth2ProductionTest, RequestTokenValidCredentials) {
  auto barrier = std::make_shared<std::promise<void>>();
  token_endpoint_.RequestToken(
      authentication::TokenRequest{},
      [barrier](authentication::TokenResponse token_response) {
#if OAUTH2_TEST_DEBUG_OUTPUT
        std::cout << "Is successful : " << token_response.IsSuccessful()
                  << std::endl;
        if (token_response) {
          std::cout << "Access Token : "
                    << token_response.GetResult().GetAccessToken() << std::endl;
          std::cout << "Expiry Time : "
                    << token_response.GetResult().GetExpiryTime() << std::endl;

        } else {
          std::cout << "Http Status : "
                    << token_response.GetError().GetHttpStatusCode()
                    << std::endl;
          std::cout << "Error ID : " << token_response.GetError().GetErrorCode()
                    << std::endl;
          std::cout << "Error Message : "
                    << token_response.GetError().GetMessage() << std::endl;
        }
#endif
        ASSERT_TRUE(token_response);
        EXPECT_GT(token_response.GetResult().GetAccessToken().length(), 42u);
        EXPECT_GT(token_response.GetResult().GetExpiryTime(), time(nullptr));
        barrier->set_value();
      });
  EXPECT_EQ(std::future_status::ready,
            barrier->get_future().wait_for(kTestMaxExecutionTime));
}

TEST_F(HereAccountOuauth2ProductionTest, RequestTokenValidCredentialsFuture) {
  EXPECT_EQ(std::future_status::ready,
            token_endpoint_.RequestToken().wait_for(kTestMaxExecutionTime));
  auto token_response = token_endpoint_.RequestToken().get();

  ASSERT_TRUE(token_response);
  EXPECT_GT(token_response.GetResult().GetAccessToken().length(), 42u);
  EXPECT_GT(token_response.GetResult().GetExpiryTime(), time(nullptr));
}

TEST_F(HereAccountOuauth2ProductionTest, RequestTokenBadAccessKey) {
  auto settings = settings_;
  settings.credentials = {"BAD", CustomParameters::getArgument(
                                     "integration_production_service_secret")};
  auto bad_token_endpoint = authentication::TokenEndpoint(settings);

  auto barrier = std::make_shared<std::promise<void>>();
  bad_token_endpoint.RequestToken(
      authentication::TokenRequest{},
      [barrier](authentication::TokenResponse token_response) {
        ASSERT_FALSE(token_response);
        EXPECT_EQ(token_response.GetError().GetHttpStatusCode(),
                  http::HttpStatusCode::UNAUTHORIZED);
        EXPECT_THAT(token_response.GetError().GetMessage(),
                    IsUnauthorizedMessage);
        barrier->set_value();
      });
  EXPECT_EQ(std::future_status::ready,
            barrier->get_future().wait_for(kTestMaxExecutionTime));
}

TEST_F(HereAccountOuauth2ProductionTest, RequestTokenBadAccessSecret) {
  auto settings = settings_;
  settings.credentials = {
      CustomParameters::getArgument("integration_production_service_id"),
      "BAD"};
  auto bad_token_endpoint = authentication::TokenEndpoint(settings);

  auto barrier = std::make_shared<std::promise<void>>();
  bad_token_endpoint.RequestToken(
      authentication::TokenRequest{},
      [barrier](authentication::TokenResponse token_response) {
        ASSERT_FALSE(token_response);
        EXPECT_EQ(token_response.GetError().GetHttpStatusCode(),
                  http::HttpStatusCode::UNAUTHORIZED);
        EXPECT_THAT(token_response.GetError().GetMessage(),
                    IsUnauthorizedMessage);
        barrier->set_value();
      });
  EXPECT_EQ(std::future_status::ready,
            barrier->get_future().wait_for(kTestMaxExecutionTime));
}

TEST_F(HereAccountOuauth2ProductionTest, RequestTokenBadTokenUrl) {
  authentication::Settings badSettings(
      {CustomParameters::getArgument("integration_production_service_id"),
       CustomParameters::getArgument("integration_production_service_secret")});
  badSettings.token_endpoint_url = "BAD";
  badSettings.network_request_handler = settings_.network_request_handler;
  auto bad_token_endpoint = authentication::TokenEndpoint(badSettings);

  auto barrier = std::make_shared<std::promise<void>>();
  bad_token_endpoint.RequestToken(
      authentication::TokenRequest{},
      [barrier](authentication::TokenResponse token_response) {
        EXPECT_FALSE(token_response);
        barrier->set_value();
      });
  EXPECT_EQ(std::future_status::ready,
            barrier->get_future().wait_for(kTestMaxExecutionTime));
}

TEST_F(HereAccountOuauth2ProductionTest, RequestTokenValidExpiry) {
  auto barrier = std::make_shared<std::promise<void>>();
  token_endpoint_.RequestToken(
      authentication::TokenRequest{std::chrono::minutes(1)},
      [barrier](authentication::TokenResponse token_response) {
        ASSERT_TRUE(token_response);
        EXPECT_LT(token_response.GetResult().GetExpiryTime(),
                  time(nullptr) + 120);
        barrier->set_value();
      });
  EXPECT_EQ(std::future_status::ready,
            barrier->get_future().wait_for(kTestMaxExecutionTime));
}

TEST_F(HereAccountOuauth2ProductionTest, RequestTokenConcurrent) {
  std::thread threads[5];
  auto access_tokens = std::vector<std::string>();
  auto delta_sum = std::chrono::high_resolution_clock::duration::zero();
  std::mutex global_state_mutex;

  auto start_total_time = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 5; i++) {
    threads[i] = std::thread([&]() {
      auto barrier = std::make_shared<std::promise<void>>();
      auto start = std::chrono::high_resolution_clock::now();
      token_endpoint_.RequestToken(
          authentication::TokenRequest{},
          [&, barrier, start](authentication::TokenResponse token_response) {
            auto delta = std::chrono::high_resolution_clock::now() - start;
            ASSERT_TRUE(token_response)
                << token_response.GetError().GetMessage();
            EXPECT_FALSE(token_response.GetResult().GetAccessToken().empty());
            {
              std::lock_guard<std::mutex> guard(global_state_mutex);
              delta_sum += delta;
              access_tokens.emplace_back(
                  token_response.GetResult().GetAccessToken());
            }
            barrier->set_value();
          });
      EXPECT_EQ(std::future_status::ready,
                barrier->get_future().wait_for(kTestMaxExecutionTime * 4));
    });
  }

  for (int i = 0; i < 5; i++) {
    threads[i].join();
  }

  auto delta_total_time =
      std::chrono::high_resolution_clock::now() - start_total_time;
  EXPECT_LT(delta_total_time, delta_sum)
      << "Expect token request operations to have happened in parallel";

  EXPECT_EQ(access_tokens.size(), 5u);
  auto it = std::unique(access_tokens.begin(), access_tokens.end());
  EXPECT_EQ(access_tokens.end(), it)
      << "Expected all access tokens to be unique.";
}

TEST_F(HereAccountOuauth2ProductionTest, RequestTokenConcurrentFuture) {
  std::thread threads[5];
  auto access_tokens = std::vector<std::string>();
  auto delta_sum = std::chrono::high_resolution_clock::duration::zero();
  std::mutex global_state_mutex;

  auto start_total_time = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 5; i++) {
    threads[i] = std::thread([&]() {
      auto start = std::chrono::high_resolution_clock::now();
      auto token_response = token_endpoint_.RequestToken().get();
      auto delta = std::chrono::high_resolution_clock::now() - start;
      ASSERT_TRUE(token_response) << token_response.GetError().GetMessage();
      EXPECT_FALSE(token_response.GetResult().GetAccessToken().empty());
      {
        std::lock_guard<std::mutex> guard(global_state_mutex);
        delta_sum += delta;
        access_tokens.emplace_back(token_response.GetResult().GetAccessToken());
      }
    });
  }
  auto delta_total_time =
      std::chrono::high_resolution_clock::now() - start_total_time;

  for (int i = 0; i < 5; i++) {
    threads[i].join();
  }

  EXPECT_LT(delta_total_time, delta_sum)
      << "Expect token request operations to have happened in parallel";

  EXPECT_EQ(access_tokens.size(), 5u);
  auto it = std::unique(access_tokens.begin(), access_tokens.end());
  EXPECT_EQ(access_tokens.end(), it)
      << "Expected all access tokens to be unique.";
}

TEST_F(HereAccountOuauth2ProductionTest, NetworkProxySettings) {
  authentication::Settings settings(
      {CustomParameters::getArgument("integration_production_service_id"),
       CustomParameters::getArgument("integration_production_service_secret")});
  olp::http::NetworkProxySettings proxy_settings;
  proxy_settings.WithHostname("$.?");
  proxy_settings.WithPort(42);
  proxy_settings.WithType(olp::http::NetworkProxySettings::Type::SOCKS4);
  settings.network_proxy_settings = proxy_settings;
  settings.network_request_handler = settings_.network_request_handler;

  auto bad_token_endpoint = authentication::TokenEndpoint(settings);

  auto barrier = std::make_shared<std::promise<void>>();
  bad_token_endpoint.RequestToken(
      authentication::TokenRequest{},
      [barrier](authentication::TokenResponse token_response) {
        // Bad proxy error code and message varies by platform
        EXPECT_FALSE(token_response);
        //        EXPECT_LT(token_response.GetError().GetErrorCode(), 0);
        //        EXPECT_FALSE(token_response.GetError().GetMessage().empty());
        //        EXPECT_EQ(token_response.GetError().GetErrorCode(), 0);
        barrier->set_value();
      });
  EXPECT_EQ(std::future_status::ready,
            barrier->get_future().wait_for(kTestMaxExecutionTime));
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

TEST_F(HereAccountOuauth2ProductionTest, AutoRefreshingTokenValidRequest) {
  TestAutoRefreshingTokenValidRequest(
      token_endpoint_,
      [](const authentication::AutoRefreshingToken& auto_token) {
        return GetTokenFromSyncRequest(auto_token);
      });

  TestAutoRefreshingTokenValidRequest(
      token_endpoint_,
      [](const authentication::AutoRefreshingToken& auto_token) {
        return GetTokenFromAsyncRequest(auto_token);
      });
}

TEST_F(HereAccountOuauth2ProductionTest, AutoRefreshingTokenInvalidRequest) {
  TestAutoRefreshingTokenInvalidRequest(
      s_network_, [](const authentication::AutoRefreshingToken& auto_token) {
        return GetTokenFromSyncRequest(auto_token);
      });

  TestAutoRefreshingTokenInvalidRequest(
      s_network_, [](const authentication::AutoRefreshingToken& auto_token) {
        return GetTokenFromAsyncRequest(auto_token);
      });
}

TEST_F(HereAccountOuauth2ProductionTest, AutoRefreshingTokenReuseToken) {
  TestAutoRefreshingTokenReuseToken(
      token_endpoint_,
      [](const authentication::AutoRefreshingToken& auto_token) {
        return GetTokenFromSyncRequest(auto_token);
      });

  TestAutoRefreshingTokenReuseToken(
      token_endpoint_,
      [](const authentication::AutoRefreshingToken& auto_token) {
        return GetTokenFromAsyncRequest(auto_token);
      });
}

TEST_F(HereAccountOuauth2ProductionTest, AutoRefreshingTokenForceRefresh) {
  TestAutoRefreshingTokenForceRefresh(
      token_endpoint_, [](const authentication::AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromSyncRequest(auto_token, minimum_validity);
      });

  TestAutoRefreshingTokenForceRefresh(
      token_endpoint_, [](const authentication::AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromAsyncRequest(auto_token, minimum_validity);
      });
}

TEST_F(HereAccountOuauth2ProductionTest,
       AutoRefreshingTokenExpiresInRefreshSync) {
  TestAutoRefreshingTokenExpiresInRefresh(
      token_endpoint_,
      [](const authentication::AutoRefreshingToken& auto_token) {
        return GetTokenFromSyncRequest(auto_token);
      });
}

TEST_F(HereAccountOuauth2ProductionTest,
       AutoRefreshingTokenExpiresInRefreshAsync) {
  TestAutoRefreshingTokenExpiresInRefresh(
      token_endpoint_,
      [](const authentication::AutoRefreshingToken& auto_token) {
        return GetTokenFromAsyncRequest(auto_token);
      });
}

TEST_F(HereAccountOuauth2ProductionTest,
       AutoRefreshingTokenExpiresDoNotRefresh) {
  TestAutoRefreshingTokenExpiresDoNotRefresh(
      token_endpoint_,
      [](const authentication::AutoRefreshingToken& auto_token) {
        return GetTokenFromSyncRequest(auto_token);
      });

  TestAutoRefreshingTokenExpiresDoNotRefresh(
      token_endpoint_,
      [](const authentication::AutoRefreshingToken& auto_token) {
        return GetTokenFromAsyncRequest(auto_token);
      });
}

TEST_F(HereAccountOuauth2ProductionTest, AutoRefreshingTokenExpiresDoRefresh) {
  TestAutoRefreshingTokenExpiresDoRefresh(
      token_endpoint_, [](const authentication::AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromSyncRequest(auto_token, minimum_validity);
      });

  TestAutoRefreshingTokenExpiresDoRefresh(
      token_endpoint_, [](const authentication::AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromAsyncRequest(auto_token, minimum_validity);
      });
}

TEST_F(HereAccountOuauth2ProductionTest, AutoRefreshingTokenExpiresInAnHour) {
  TestAutoRefreshingTokenExpiresInAnHour(
      token_endpoint_, [](const authentication::AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromSyncRequest(auto_token, minimum_validity);
      });

  TestAutoRefreshingTokenExpiresInAnHour(
      token_endpoint_, [](const authentication::AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromAsyncRequest(auto_token, minimum_validity);
      });
}

TEST_F(HereAccountOuauth2ProductionTest, AutoRefreshingTokenExpiresInASecond) {
  TestAutoRefreshingTokenExpiresInASecond(
      token_endpoint_, [](const authentication::AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromSyncRequest(auto_token, minimum_validity);
      });

  TestAutoRefreshingTokenExpiresInASecond(
      token_endpoint_, [](const authentication::AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromAsyncRequest(auto_token, minimum_validity);
      });
}

TEST_F(HereAccountOuauth2ProductionTest, AutoRefreshingTokenMultiThread) {
  TestAutoRefreshingTokenMultiThread(
      token_endpoint_,
      [](const authentication::AutoRefreshingToken& auto_token) {
        return GetTokenFromSyncRequest(auto_token);
      });

  TestAutoRefreshingTokenMultiThread(
      token_endpoint_,
      [](const authentication::AutoRefreshingToken& auto_token) {
        return GetTokenFromAsyncRequest(auto_token);
      });
}

}  // namespace

PORTING_POP_WARNINGS()
