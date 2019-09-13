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

#include "AuthenticationOnlineTest.h"

#include <olp/authentication/AuthenticationClient.h>
#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/AutoRefreshingToken.h>
#include <olp/authentication/ErrorResponse.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenEndpoint.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/authentication/TokenRequest.h>
#include <olp/authentication/TokenResult.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/NetworkProxySettings.h>

using namespace olp::authentication;

namespace {
constexpr auto kTestMaxExecutionTime = std::chrono::seconds(30);

TokenEndpoint::TokenResponse GetTokenFromSyncRequest(
    const AutoRefreshingToken& auto_token,
    const std::chrono::seconds minimum_validity =
        kDefaultMinimumValiditySeconds) {
  return auto_token.GetToken(minimum_validity);
}

TokenEndpoint::TokenResponse GetTokenFromSyncRequest(
    olp::client::CancellationToken& cancellation_token,
    const AutoRefreshingToken& auto_token,
    const std::chrono::seconds minimum_validity =
        kDefaultMinimumValiditySeconds) {
  return auto_token.GetToken(cancellation_token, minimum_validity);
}

TokenEndpoint::TokenResponse GetTokenFromAsyncRequest(
    const AutoRefreshingToken& auto_token,
    const std::chrono::seconds minimum_validity =
        kDefaultMinimumValiditySeconds) {
  std::promise<TokenEndpoint::TokenResponse> promise;
  auto future = promise.get_future();
  auto_token.GetToken(
      [&promise](TokenEndpoint::TokenResponse tokenResponse) {
        promise.set_value(tokenResponse);
      },
      minimum_validity);
  return future.get();
}

TokenEndpoint::TokenResponse GetTokenFromAsyncRequest(
    olp::client::CancellationToken& cancellation_token,
    const AutoRefreshingToken& auto_token,
    const std::chrono::seconds minimum_validity =
        kDefaultMinimumValiditySeconds) {
  std::promise<TokenEndpoint::TokenResponse> promise;
  auto future = promise.get_future();
  cancellation_token = auto_token.GetToken(
      [&promise](TokenEndpoint::TokenResponse tokenResponse) {
        promise.set_value(tokenResponse);
      },
      minimum_validity);
  return future.get();
}

void TestAutoRefreshingTokenValidRequest(
    TokenEndpoint& token_endpoint, std::function<TokenEndpoint::TokenResponse(
                                       const AutoRefreshingToken& auto_token)>
                                       func) {
  auto tokenResponse = func(token_endpoint.RequestAutoRefreshingToken());
  EXPECT_TRUE(tokenResponse.IsSuccessful());
  EXPECT_GT(tokenResponse.GetResult().GetAccessToken().length(), 42u);
  EXPECT_GT(tokenResponse.GetResult().GetExpiryTime(), time(nullptr));
}

void TestAutoRefreshingTokenCancel(
    TokenEndpoint& token_endpoint,
    std::function<TokenEndpoint::TokenResponse(
        olp::client::CancellationToken& cancellation_token,
        const AutoRefreshingToken& auto_token,
        const std::chrono::seconds minimum_validity)>
        func) {
  auto auto_token = token_endpoint.RequestAutoRefreshingToken();

  std::thread threads[2];
  auto token_responses = std::vector<TokenEndpoint::TokenResponse>();
  std::mutex token_responses_mutex;
  olp::client::CancellationToken cancellation_token;

  // get a first refresh token and wait for it to come back.
  token_responses.emplace_back(
      func(cancellation_token, auto_token, std::chrono::minutes(5)));
  ASSERT_EQ(token_responses.size(), 1u);

  // get a second forced refresh token
  threads[0] = std::thread([&]() {
    std::lock_guard<std::mutex> guard(token_responses_mutex);
    token_responses.emplace_back(
        func(cancellation_token, auto_token, kForceRefresh));
  });

  // Cancel the refresh from another thread so that the response will come back
  // with the same old token
  threads[1] = std::thread([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cancellation_token.cancel();
  });
  threads[0].join();
  threads[1].join();

  ASSERT_EQ(token_responses.size(), 2u);
  ASSERT_EQ(token_responses[0].GetResult().GetAccessToken(),
            token_responses[1].GetResult().GetAccessToken());
  ASSERT_LE(abs(token_responses[1].GetResult().GetExpiryTime() -
                token_responses[0].GetResult().GetExpiryTime()),
            10);
}
}  // namespace

class HereAccountOauth2IntegrationOnlineTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    s_network_ = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler(1);
  }

  static void TearDownTestSuite() { s_network_.reset(); }

  HereAccountOauth2IntegrationOnlineTest()
      : settings_([]() -> Settings {
          Settings settings;
          settings.task_scheduler = olp::client::OlpClientSettingsFactory::
              CreateDefaultTaskScheduler();
          settings.network_request_handler = s_network_;
          return settings;
        }()),
        token_endpoint_(
            TokenEndpoint(AuthenticationCredentials(
                              CustomParameters::getArgument(
                                  "integration_production_service_id"),
                              CustomParameters::getArgument(
                                  "integration_production_service_secret")),
                          settings_)) {}

  Settings settings_;
  TokenEndpoint token_endpoint_;

 protected:
  static std::shared_ptr<olp::http::Network> s_network_;
};

std::shared_ptr<olp::http::Network>
    HereAccountOauth2IntegrationOnlineTest::s_network_;

TEST_F(HereAccountOauth2IntegrationOnlineTest,
       TokenProviderValidCredentialsValid) {
  TokenProviderDefault prov{
      CustomParameters::getArgument("integration_production_service_id"),
      CustomParameters::getArgument("integration_production_service_secret"),
      settings_};
  ASSERT_TRUE(prov);
  ASSERT_NE("", prov());
  ASSERT_EQ(olp::http::HttpStatusCode::OK, prov.GetHttpStatusCode());

  ASSERT_TRUE(prov);
  ASSERT_NE("", prov());
  ASSERT_EQ(olp::http::HttpStatusCode::OK, prov.GetHttpStatusCode());
}

TEST_F(HereAccountOauth2IntegrationOnlineTest,
       TokenProviderValidCredentialsInvalid) {
  auto tokenProviderTest = [this](std::string key, std::string secret) {
    TokenProviderDefault prov{key, secret, settings_};
    ASSERT_FALSE(prov);
    ASSERT_EQ("", prov());
    ASSERT_EQ(401300, (int)prov.GetErrorResponse().code);
    ASSERT_EQ(401, prov.GetHttpStatusCode());
  };

  tokenProviderTest("BAD", CustomParameters::getArgument(
                               "integration_production_service_secret"));
  tokenProviderTest(
      CustomParameters::getArgument("integration_production_service_id"),
      "BAD");
  tokenProviderTest("BAD", "BAD");
}

TEST_F(HereAccountOauth2IntegrationOnlineTest, RequestTokenValidCredentials) {
  auto barrier = std::make_shared<std::promise<void> >();
  token_endpoint_.RequestToken(
      TokenRequest{}, [barrier](TokenEndpoint::TokenResponse tokenResponse) {
#if OAUTH2_TEST_DEBUG_OUTPUT
        std::cout << "Is successful : " << tokenResponse.IsSuccessful()
                  << std::endl;
        if (tokenResponse.IsSuccessful()) {
          std::cout << "Access Token : "
                    << tokenResponse.GetResult().GetAccessToken() << std::endl;
          std::cout << "Expiry Time : "
                    << tokenResponse.GetResult().GetExpiryTime() << std::endl;

        } else {
          std::cout << "Http Status : "
                    << tokenResponse.GetError().GetHttpStatusCode()
                    << std::endl;
          std::cout << "Error ID : " << tokenResponse.GetError().GetErrorCode()
                    << std::endl;
          std::cout << "Error Message : "
                    << tokenResponse.GetError().GetMessage() << std::endl;
        }
#endif
        EXPECT_TRUE(tokenResponse.IsSuccessful());
        EXPECT_GT(tokenResponse.GetResult().GetAccessToken().length(), 42u);
        EXPECT_GT(tokenResponse.GetResult().GetExpiryTime(), time(nullptr));
        barrier->set_value();
      });
  EXPECT_EQ(std::future_status::ready,
            barrier->get_future().wait_for(kTestMaxExecutionTime));
}

TEST_F(HereAccountOauth2IntegrationOnlineTest,
       RequestTokenValidCredentialsFuture) {
  EXPECT_EQ(std::future_status::ready,
            token_endpoint_.RequestToken().wait_for(kTestMaxExecutionTime));
  auto tokenResponse = token_endpoint_.RequestToken().get();

  EXPECT_TRUE(tokenResponse.IsSuccessful());
  EXPECT_GT(tokenResponse.GetResult().GetAccessToken().length(), 42u);
  EXPECT_GT(tokenResponse.GetResult().GetExpiryTime(), time(nullptr));
}

TEST_F(HereAccountOauth2IntegrationOnlineTest, RequestTokenBadAccessKey) {
  auto badTokenEndpoint =
      TokenEndpoint(AuthenticationCredentials(
                        "BAD", CustomParameters::getArgument(
                                   "integration_production_service_secret")),
                    settings_);

  auto barrier = std::make_shared<std::promise<void> >();
  badTokenEndpoint.RequestToken(
      TokenRequest{}, [barrier](TokenEndpoint::TokenResponse tokenResponse) {
        EXPECT_TRUE(tokenResponse.IsSuccessful());
        EXPECT_EQ(tokenResponse.GetResult().GetHttpStatus(), 401);
        EXPECT_GT(tokenResponse.GetResult().GetErrorResponse().code, 0u);
        barrier->set_value();
      });
  EXPECT_EQ(std::future_status::ready,
            barrier->get_future().wait_for(kTestMaxExecutionTime));
}

TEST_F(HereAccountOauth2IntegrationOnlineTest, RequestTokenBadAccessSecret) {
  auto badTokenEndpoint = TokenEndpoint(
      AuthenticationCredentials(
          CustomParameters::getArgument("integration_production_service_id"),
          "BAD"),
      settings_);

  auto barrier = std::make_shared<std::promise<void> >();
  badTokenEndpoint.RequestToken(
      TokenRequest{}, [barrier](TokenEndpoint::TokenResponse tokenResponse) {
        EXPECT_TRUE(tokenResponse.IsSuccessful());
        EXPECT_EQ(tokenResponse.GetResult().GetHttpStatus(), 401);
        EXPECT_GT(tokenResponse.GetResult().GetErrorResponse().code, 0u);
        barrier->set_value();
      });
  EXPECT_EQ(std::future_status::ready,
            barrier->get_future().wait_for(kTestMaxExecutionTime));
}

TEST_F(HereAccountOauth2IntegrationOnlineTest, RequestTokenBadTokenUrl) {
  Settings badSettings;
  badSettings.token_endpoint_url = "BAD";
  badSettings.network_request_handler = settings_.network_request_handler;
  auto badTokenEndpoint = TokenEndpoint(
      AuthenticationCredentials(
          CustomParameters::getArgument("integration_production_service_id"),
          CustomParameters::getArgument(
              "integration_production_service_secret")),
      badSettings);

  auto barrier = std::make_shared<std::promise<void> >();
  badTokenEndpoint.RequestToken(
      TokenRequest{}, [barrier](TokenEndpoint::TokenResponse tokenResponse) {
        EXPECT_FALSE(tokenResponse.IsSuccessful());
        barrier->set_value();
      });
  EXPECT_EQ(std::future_status::ready,
            barrier->get_future().wait_for(kTestMaxExecutionTime));
}

TEST_F(HereAccountOauth2IntegrationOnlineTest, RequestTokenValidExpiry) {
  auto barrier = std::make_shared<std::promise<void> >();
  token_endpoint_.RequestToken(
      TokenRequest{std::chrono::minutes(1)},
      [barrier](TokenEndpoint::TokenResponse tokenResponse) {
        EXPECT_TRUE(tokenResponse.IsSuccessful());
        EXPECT_LT(tokenResponse.GetResult().GetExpiryTime(),
                  time(nullptr) + 120);
        barrier->set_value();
      });
  EXPECT_EQ(std::future_status::ready,
            barrier->get_future().wait_for(kTestMaxExecutionTime));
}

TEST_F(HereAccountOauth2IntegrationOnlineTest, RequestTokenConcurrent) {
  std::thread threads[5];
  auto accessTokens = std::vector<std::string>();
  auto deltaSum = std::chrono::high_resolution_clock::duration::zero();
  std::mutex globalStateMutex;

  auto startTotalTime = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 5; i++) {
    threads[i] = std::thread([&]() {
      auto barrier = std::make_shared<std::promise<void> >();
      auto start = std::chrono::high_resolution_clock::now();
      token_endpoint_.RequestToken(
          TokenRequest{},
          [&, barrier, start](TokenEndpoint::TokenResponse tokenResponse) {
            auto delta = std::chrono::high_resolution_clock::now() - start;
            EXPECT_TRUE(tokenResponse.IsSuccessful());
            EXPECT_FALSE(tokenResponse.GetResult().GetAccessToken().empty());
            {
              std::lock_guard<std::mutex> guard(globalStateMutex);
              deltaSum += delta;
              accessTokens.emplace_back(
                  tokenResponse.GetResult().GetAccessToken());
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

  auto deltaTotalTime =
      std::chrono::high_resolution_clock::now() - startTotalTime;
  EXPECT_LE((deltaTotalTime * 2), deltaSum)
      << "Expect token request operations to have happened in parallel";

  EXPECT_EQ(accessTokens.size(), 5u);
  auto it = std::unique(accessTokens.begin(), accessTokens.end());
  EXPECT_EQ(accessTokens.end(), it)
      << "Expected all access tokens to be unique.";
}

TEST_F(HereAccountOauth2IntegrationOnlineTest, RequestTokenConcurrentFuture) {
  std::thread threads[5];
  auto accessTokens = std::vector<std::string>();
  auto deltaSum = std::chrono::high_resolution_clock::duration::zero();
  std::mutex globalStateMutex;

  auto startTotalTime = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 5; i++) {
    threads[i] = std::thread([&]() {
      auto start = std::chrono::high_resolution_clock::now();
      auto tokenResponse = token_endpoint_.RequestToken().get();
      auto delta = std::chrono::high_resolution_clock::now() - start;
      EXPECT_TRUE(tokenResponse.IsSuccessful());
      EXPECT_FALSE(tokenResponse.GetResult().GetAccessToken().empty());
      {
        std::lock_guard<std::mutex> guard(globalStateMutex);
        deltaSum += delta;
        accessTokens.emplace_back(tokenResponse.GetResult().GetAccessToken());
      }
    });
  }
  auto deltaTotalTime =
      std::chrono::high_resolution_clock::now() - startTotalTime;

  for (int i = 0; i < 5; i++) {
    threads[i].join();
  }

  EXPECT_LE((deltaTotalTime * 2), deltaSum)
      << "Expect token request operations to have happened in parallel";

  EXPECT_EQ(accessTokens.size(), 5u);
  auto it = std::unique(accessTokens.begin(), accessTokens.end());
  EXPECT_EQ(accessTokens.end(), it)
      << "Expected all access tokens to be unique.";
}

TEST_F(HereAccountOauth2IntegrationOnlineTest, NetworkProxySettings) {
  Settings settings;
  olp::http::NetworkProxySettings proxySettings;
  proxySettings.WithHostname("$.?");
  proxySettings.WithPort(42);
  proxySettings.WithType(olp::http::NetworkProxySettings::Type::SOCKS4);
  settings.network_proxy_settings = proxySettings;
  settings.network_request_handler = settings_.network_request_handler;

  auto badTokenEndpoint = TokenEndpoint(
      AuthenticationCredentials(
          CustomParameters::getArgument("integration_production_service_id"),
          CustomParameters::getArgument(
              "integration_production_service_secret")),
      settings);

  auto barrier = std::make_shared<std::promise<void> >();
  badTokenEndpoint.RequestToken(
      TokenRequest{}, [barrier](TokenEndpoint::TokenResponse tokenResponse) {
        // Bad proxy error code and message varies by platform
        EXPECT_FALSE(tokenResponse.IsSuccessful());
        //        EXPECT_LT(tokenResponse.GetError().GetErrorCode(), 0);
        //        EXPECT_FALSE(tokenResponse.GetError().GetMessage().empty());
        //        EXPECT_EQ(tokenResponse.GetError().GetErrorCode(), 0);
        barrier->set_value();
      });
  EXPECT_EQ(std::future_status::ready,
            barrier->get_future().wait_for(kTestMaxExecutionTime));
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

TEST_F(HereAccountOauth2IntegrationOnlineTest,
       AutoRefreshingTokenValidRequest) {
  TestAutoRefreshingTokenValidRequest(
      token_endpoint_, [](const AutoRefreshingToken& auto_token) {
        return GetTokenFromSyncRequest(auto_token);
      });

  TestAutoRefreshingTokenValidRequest(
      token_endpoint_, [](const AutoRefreshingToken& auto_token) {
        return GetTokenFromAsyncRequest(auto_token);
      });
}

void testAutoRefreshingTokenInvalidRequest(
    const std::shared_ptr<olp::http::Network>& network,
    std::function<
        TokenEndpoint::TokenResponse(const AutoRefreshingToken& auto_token)>
        func) {
  auto badTokenEndpoint = TokenEndpoint(
      AuthenticationCredentials("BAD", "BAD"), [network]() -> Settings {
        Settings settings;
        settings.task_scheduler =
            olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
        settings.network_request_handler = network;
        return settings;
      }());
  auto tokenResponse = func(badTokenEndpoint.RequestAutoRefreshingToken());
  EXPECT_TRUE(tokenResponse.IsSuccessful());
  EXPECT_EQ(tokenResponse.GetResult().GetHttpStatus(), 401);
  EXPECT_GT(tokenResponse.GetResult().GetErrorResponse().code, 0u);
}

TEST_F(HereAccountOauth2IntegrationOnlineTest,
       AutoRefreshingTokenInvalidRequest) {
  testAutoRefreshingTokenInvalidRequest(
      s_network_, [](const AutoRefreshingToken& auto_token) {
        return GetTokenFromSyncRequest(auto_token);
      });

  testAutoRefreshingTokenInvalidRequest(
      s_network_, [](const AutoRefreshingToken& auto_token) {
        return GetTokenFromAsyncRequest(auto_token);
      });
}

void testAutoRefreshingTokenReuseToken(
    TokenEndpoint& token_endpoint, std::function<TokenEndpoint::TokenResponse(
                                       const AutoRefreshingToken& auto_token)>
                                       func) {
  auto auto_token = token_endpoint.RequestAutoRefreshingToken();
  auto tokenResponseOne = func(auto_token);
  auto tokenResponseTwo = func(auto_token);
  EXPECT_EQ(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
  EXPECT_EQ(tokenResponseOne.GetResult().GetExpiryTime(),
            tokenResponseTwo.GetResult().GetExpiryTime());
}

TEST_F(HereAccountOauth2IntegrationOnlineTest, AutoRefreshingTokenReuseToken) {
  testAutoRefreshingTokenReuseToken(
      token_endpoint_, [](const AutoRefreshingToken& auto_token) {
        return GetTokenFromSyncRequest(auto_token);
      });

  testAutoRefreshingTokenReuseToken(
      token_endpoint_, [](const AutoRefreshingToken& auto_token) {
        return GetTokenFromAsyncRequest(auto_token);
      });
}

void testAutoRefreshingTokenForceRefresh(
    TokenEndpoint& token_endpoint,
    std::function<TokenEndpoint::TokenResponse(
        const AutoRefreshingToken& auto_token,
        const std::chrono::seconds minimum_validity)>
        func) {
  auto auto_token = token_endpoint.RequestAutoRefreshingToken();
  auto tokenResponseOne = func(auto_token, std::chrono::minutes(5));
  auto tokenResponseTwo = func(auto_token, kForceRefresh);

  EXPECT_NE(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
}

TEST_F(HereAccountOauth2IntegrationOnlineTest,
       AutoRefreshingTokenForceRefresh) {
  testAutoRefreshingTokenForceRefresh(
      token_endpoint_, [](const AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromSyncRequest(auto_token, minimum_validity);
      });

  testAutoRefreshingTokenForceRefresh(
      token_endpoint_, [](const AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromAsyncRequest(auto_token, minimum_validity);
      });
}

void testAutoRefreshingTokenExpiresInRefresh(
    TokenEndpoint& token_endpoint, std::function<TokenEndpoint::TokenResponse(
                                       const AutoRefreshingToken& auto_token)>
                                       func) {
  auto auto_token = token_endpoint.RequestAutoRefreshingToken(
      TokenRequest{std::chrono::seconds(302)});
  auto tokenResponseOne = func(auto_token);
  std::this_thread::sleep_for(std::chrono::seconds(4));
  auto tokenResponseTwo = func(auto_token);

  EXPECT_NE(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
  EXPECT_NE(tokenResponseOne.GetResult().GetExpiryTime(),
            tokenResponseTwo.GetResult().GetExpiryTime());
}

TEST_F(HereAccountOauth2IntegrationOnlineTest,
       AutoRefreshingTokenExpiresInRefreshSync) {
  testAutoRefreshingTokenExpiresInRefresh(
      token_endpoint_, [](const AutoRefreshingToken& auto_token) {
        return GetTokenFromSyncRequest(auto_token);
      });
}

TEST_F(HereAccountOauth2IntegrationOnlineTest,
       AutoRefreshingTokenExpiresInRefreshAsync) {
  testAutoRefreshingTokenExpiresInRefresh(
      token_endpoint_, [](const AutoRefreshingToken& auto_token) {
        return GetTokenFromAsyncRequest(auto_token);
      });
}

void testAutoRefreshingTokenExpiresDoNotRefresh(
    TokenEndpoint& token_endpoint, std::function<TokenEndpoint::TokenResponse(
                                       const AutoRefreshingToken& auto_token)>
                                       func) {
  auto auto_token = token_endpoint.RequestAutoRefreshingToken(
      TokenRequest{std::chrono::seconds(305)});
  auto tokenResponseOne = func(auto_token);
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto tokenResponseTwo = func(auto_token);

  EXPECT_EQ(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
  EXPECT_EQ(tokenResponseOne.GetResult().GetExpiryTime(),
            tokenResponseTwo.GetResult().GetExpiryTime());
}

TEST_F(HereAccountOauth2IntegrationOnlineTest,
       AutoRefreshingTokenExpiresDoNotRefresh) {
  testAutoRefreshingTokenExpiresDoNotRefresh(
      token_endpoint_, [](const AutoRefreshingToken& auto_token) {
        return GetTokenFromSyncRequest(auto_token);
      });

  testAutoRefreshingTokenExpiresDoNotRefresh(
      token_endpoint_, [](const AutoRefreshingToken& auto_token) {
        return GetTokenFromAsyncRequest(auto_token);
      });
}

void testAutoRefreshingTokenExpiresDoRefresh(
    TokenEndpoint& token_endpoint,
    std::function<TokenEndpoint::TokenResponse(
        const AutoRefreshingToken& auto_token,
        const std::chrono::seconds minimum_validity)>
        func) {
  auto auto_token = token_endpoint.RequestAutoRefreshingToken(
      TokenRequest{std::chrono::seconds(1)});  // 1 second
  auto tokenResponseOne =
      func(auto_token, std::chrono::seconds(1));  // 1 sec validity window,
                                                  // shot enough to trigger
                                                  // a refresh
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto tokenResponseTwo =
      func(auto_token, std::chrono::seconds(1));  // 1 sec validity window,
                                                  // shot enough to trigger
                                                  // a refresh

  EXPECT_NE(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
  EXPECT_NE(tokenResponseOne.GetResult().GetExpiryTime(),
            tokenResponseTwo.GetResult().GetExpiryTime());
}

TEST_F(HereAccountOauth2IntegrationOnlineTest,
       AutoRefreshingTokenExpiresDoRefresh) {
  testAutoRefreshingTokenExpiresDoRefresh(
      token_endpoint_, [](const AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromSyncRequest(auto_token, minimum_validity);
      });

  testAutoRefreshingTokenExpiresDoRefresh(
      token_endpoint_, [](const AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromAsyncRequest(auto_token, minimum_validity);
      });
}

void testAutoRefreshingTokenExpiresInAnHour(
    TokenEndpoint& token_endpoint,
    std::function<TokenEndpoint::TokenResponse(
        const AutoRefreshingToken& auto_token,
        const std::chrono::seconds minimum_validity)>
        func) {
  auto auto_token = token_endpoint.RequestAutoRefreshingToken(
      TokenRequest{std::chrono::hours(1)});
  auto tokenResponseOne = func(auto_token, std::chrono::seconds(1));
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto tokenResponseTwo = func(auto_token, std::chrono::seconds(1));

  EXPECT_EQ(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
  EXPECT_EQ(tokenResponseOne.GetResult().GetExpiryTime(),
            tokenResponseTwo.GetResult().GetExpiryTime());
}

TEST_F(HereAccountOauth2IntegrationOnlineTest,
       AutoRefreshingTokenExpiresInAnHour) {
  testAutoRefreshingTokenExpiresInAnHour(
      token_endpoint_, [](const AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromSyncRequest(auto_token, minimum_validity);
      });

  testAutoRefreshingTokenExpiresInAnHour(
      token_endpoint_, [](const AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromAsyncRequest(auto_token, minimum_validity);
      });
}

void testAutoRefreshingTokenExpiresInASecond(
    TokenEndpoint& token_endpoint,
    std::function<TokenEndpoint::TokenResponse(
        const AutoRefreshingToken& auto_token,
        const std::chrono::seconds minimum_validity)>
        func) {
  auto auto_token = token_endpoint.RequestAutoRefreshingToken(
      TokenRequest{std::chrono::seconds(1)});
  auto tokenResponseOne = func(auto_token, std::chrono::seconds(1));
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto tokenResponseTwo = func(auto_token, std::chrono::seconds(1));

  EXPECT_NE(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
  EXPECT_NE(tokenResponseOne.GetResult().GetExpiryTime(),
            tokenResponseTwo.GetResult().GetExpiryTime());
}

TEST_F(HereAccountOauth2IntegrationOnlineTest,
       AutoRefreshingTokenExpiresInASecond) {
  testAutoRefreshingTokenExpiresInASecond(
      token_endpoint_, [](const AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromSyncRequest(auto_token, minimum_validity);
      });

  testAutoRefreshingTokenExpiresInASecond(
      token_endpoint_, [](const AutoRefreshingToken& auto_token,
                          const std::chrono::seconds minimum_validity) {
        return GetTokenFromAsyncRequest(auto_token, minimum_validity);
      });
}

void testAutoRefreshingTokenMultiThread(
    TokenEndpoint& token_endpoint, std::function<TokenEndpoint::TokenResponse(
                                       const AutoRefreshingToken& auto_token)>
                                       func) {
  auto auto_token = token_endpoint.RequestAutoRefreshingToken();

  std::thread threads[5];
  auto token_responses = std::vector<TokenEndpoint::TokenResponse>();
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

TEST_F(HereAccountOauth2IntegrationOnlineTest, AutoRefreshingTokenMultiThread) {
  testAutoRefreshingTokenMultiThread(
      token_endpoint_, [](const AutoRefreshingToken& auto_token) {
        return GetTokenFromSyncRequest(auto_token);
      });

  testAutoRefreshingTokenMultiThread(
      token_endpoint_, [](const AutoRefreshingToken& auto_token) {
        return GetTokenFromAsyncRequest(auto_token);
      });
}
