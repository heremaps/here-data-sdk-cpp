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
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/NetworkProxySettings.h>
#include "AuthenticationTests.h"

namespace {
constexpr auto kTestMaxExecutionTime = std::chrono::seconds(30);
}  // namespace

using namespace olp::authentication;

TokenEndpoint::TokenResponse getTokenFromSyncRequest(
    const AutoRefreshingToken& autoToken,
    const std::chrono::seconds minimumValidity =
        kDefaultMinimumValiditySeconds) {
  return autoToken.GetToken(minimumValidity);
}

TokenEndpoint::TokenResponse getTokenFromSyncRequest(
    olp::client::CancellationToken& cancellationToken,
    const AutoRefreshingToken& autoToken,
    const std::chrono::seconds minimumValidity =
        kDefaultMinimumValiditySeconds) {
  return autoToken.GetToken(cancellationToken, minimumValidity);
}

TokenEndpoint::TokenResponse getTokenFromAsyncRequest(
    const AutoRefreshingToken& autoToken,
    const std::chrono::seconds minimumValidity =
        kDefaultMinimumValiditySeconds) {
  std::promise<TokenEndpoint::TokenResponse> promise;
  auto future = promise.get_future();
  autoToken.GetToken(
      [&promise](TokenEndpoint::TokenResponse tokenResponse) {
        promise.set_value(tokenResponse);
      },
      minimumValidity);
  return future.get();
}

TokenEndpoint::TokenResponse getTokenFromAsyncRequest(
    olp::client::CancellationToken& cancellationToken,
    const AutoRefreshingToken& autoToken,
    const std::chrono::seconds minimumValidity =
        kDefaultMinimumValiditySeconds) {
  std::promise<TokenEndpoint::TokenResponse> promise;
  auto future = promise.get_future();
  cancellationToken = autoToken.GetToken(
      [&promise](TokenEndpoint::TokenResponse tokenResponse) {
        promise.set_value(tokenResponse);
      },
      minimumValidity);
  return future.get();
}

void testAutoRefreshingTokenValidRequest(
    TokenEndpoint& tokenEndpoint, std::function<TokenEndpoint::TokenResponse(
                                      const AutoRefreshingToken& autoToken)>
                                      func) {
  auto tokenResponse = func(tokenEndpoint.RequestAutoRefreshingToken());
  EXPECT_TRUE(tokenResponse.IsSuccessful());
  EXPECT_GT(tokenResponse.GetResult().GetAccessToken().length(), 42u);
  EXPECT_GT(tokenResponse.GetResult().GetExpiryTime(), time(nullptr));
}

void testAutoRefreshingTokenCancel(
    TokenEndpoint& tokenEndpoint,
    std::function<TokenEndpoint::TokenResponse(
        olp::client::CancellationToken& cancellationToken,
        const AutoRefreshingToken& autoToken,
        const std::chrono::seconds minimumValidity)>
        func) {
  auto autoToken = tokenEndpoint.RequestAutoRefreshingToken();

  std::thread threads[2];
  auto tokenResponses = std::vector<TokenEndpoint::TokenResponse>();
  std::mutex tokenResponsesMutex;
  olp::client::CancellationToken cancellationToken;

  // get a first refresh token and wait for it to come back.
  tokenResponses.emplace_back(
      func(cancellationToken, autoToken, std::chrono::minutes(5)));
  ASSERT_EQ(tokenResponses.size(), 1u);

  // get a second forced refresh token
  threads[0] = std::thread([&]() {
    std::lock_guard<std::mutex> guard(tokenResponsesMutex);
    tokenResponses.emplace_back(
        func(cancellationToken, autoToken, kForceRefresh));
  });

  // Cancel the refresh from another thread so that the response will come back
  // with the same old token
  threads[1] = std::thread([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cancellationToken.cancel();
  });
  threads[0].join();
  threads[1].join();

  ASSERT_EQ(tokenResponses.size(), 2u);
  ASSERT_EQ(tokenResponses[0].GetResult().GetAccessToken(),
            tokenResponses[1].GetResult().GetAccessToken());
  ASSERT_LE(abs(tokenResponses[1].GetResult().GetExpiryTime() -
                tokenResponses[0].GetResult().GetExpiryTime()),
            10);
}

class TestHereAccountOauth2IntegrationOffline
    : public AuthenticationOfflineTest {};

TEST_F(TestHereAccountOauth2IntegrationOffline, AutoRefreshingTokenCancelSync) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .Times(2)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << response_1;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::OK)
                     .WithError(ERROR_OK));
        if (data_callback) {
          auto raw = const_cast<char*>(response_1.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0, response_1.size());
        }

        return olp::http::SendOutcome(request_id);
      });

  Settings settings;
  settings.network_request_handler = network_mock_;
  TokenEndpoint tokenEndpoint(
      AuthenticationCredentials(
          CustomParameters::getArgument("integration_production_service_id"),
          CustomParameters::getArgument(
              "integration_production_service_secret")),
      settings);

  testAutoRefreshingTokenCancel(
      tokenEndpoint, [](olp::client::CancellationToken& cancellationToken,
                        const AutoRefreshingToken& autoToken,
                        const std::chrono::seconds minimumValidity) {
        return getTokenFromSyncRequest(cancellationToken, autoToken,
                                       minimumValidity)
            .GetResult();
      });
}

TEST_F(TestHereAccountOauth2IntegrationOffline,
       AutoRefreshingTokenCancelAsync) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .Times(2)
      .WillRepeatedly([&](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback) {
        olp::http::RequestId request_id(5);
        if (payload) {
          *payload << response_1;
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(request_id)
                     .WithStatus(olp::http::HttpStatusCode::OK)
                     .WithError(ERROR_OK));
        if (data_callback) {
          auto raw = const_cast<char*>(response_1.c_str());
          data_callback(reinterpret_cast<uint8_t*>(raw), 0, response_1.size());
        }

        return olp::http::SendOutcome(request_id);
      });

  Settings settings;
  settings.network_request_handler = network_mock_;
  TokenEndpoint tokenEndpoint(
      AuthenticationCredentials(
          CustomParameters::getArgument("integration_production_service_id"),
          CustomParameters::getArgument(
              "integration_production_service_secret")),
      settings);

  testAutoRefreshingTokenCancel(
      tokenEndpoint, [](olp::client::CancellationToken& cancellationToken,
                        const AutoRefreshingToken& autoToken,
                        const std::chrono::seconds minimumValidity) {
        return getTokenFromAsyncRequest(cancellationToken, autoToken,
                                        minimumValidity)
            .GetResult();
      });
}

class TestHereAccountOauth2IntegrationOnline : public ::testing::Test {
 protected:
  TestHereAccountOauth2IntegrationOnline()
      : m_tokenEndpoint(TokenEndpoint(
            AuthenticationCredentials(
                CustomParameters::getArgument(
                    "integration_production_service_id"),
                CustomParameters::getArgument(
                    "integration_production_service_secret")),
            []() -> Settings {
              Settings settings;
              settings.task_scheduler = olp::client::OlpClientSettingsFactory::
                  CreateDefaultTaskScheduler();
              settings.network_request_handler =
                  olp::client::OlpClientSettingsFactory::
                      CreateDefaultNetworkRequestHandler();
              return settings;
            }())) {}

  TokenEndpoint m_tokenEndpoint;
};

TEST_F(TestHereAccountOauth2IntegrationOnline,
       TokenProviderValidCredentialsValid) {
  TokenProviderDefault prov{
      CustomParameters::getArgument("integration_production_service_id"),
      CustomParameters::getArgument("integration_production_service_secret"),
      []() -> Settings {
        Settings settings;
        settings.task_scheduler =
            olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
        settings.network_request_handler = olp::client::
            OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();
        return settings;
      }()};
  ASSERT_TRUE(prov);
  ASSERT_NE("", prov());
  ASSERT_EQ(200, prov.GetHttpStatusCode());

  ASSERT_TRUE(prov);
  ASSERT_NE("", prov());
  ASSERT_EQ(200, prov.GetHttpStatusCode());
}

TEST_F(TestHereAccountOauth2IntegrationOnline,
       TokenProviderValidCredentialsInvalid) {
  auto tokenProviderTest = [](std::string key, std::string secret) {
    TokenProviderDefault prov{key, secret, []() -> Settings {
                                Settings settings;
                                settings.task_scheduler =
                                    olp::client::OlpClientSettingsFactory::
                                        CreateDefaultTaskScheduler();
                                settings.network_request_handler =
                                    olp::client::OlpClientSettingsFactory::
                                        CreateDefaultNetworkRequestHandler();
                                return settings;
                              }()};
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

TEST_F(TestHereAccountOauth2IntegrationOnline, RequestTokenValidCredentials) {
  auto barrier = std::make_shared<std::promise<void> >();
  m_tokenEndpoint.RequestToken(
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

TEST_F(TestHereAccountOauth2IntegrationOnline,
       RequestTokenValidCredentialsFuture) {
  EXPECT_EQ(std::future_status::ready,
            m_tokenEndpoint.RequestToken().wait_for(kTestMaxExecutionTime));
  auto tokenResponse = m_tokenEndpoint.RequestToken().get();

  EXPECT_TRUE(tokenResponse.IsSuccessful());
  EXPECT_GT(tokenResponse.GetResult().GetAccessToken().length(), 42u);
  EXPECT_GT(tokenResponse.GetResult().GetExpiryTime(), time(nullptr));
}

TEST_F(TestHereAccountOauth2IntegrationOnline, RequestTokenBadAccessKey) {
  auto badTokenEndpoint = TokenEndpoint(
      AuthenticationCredentials("BAD",
                                CustomParameters::getArgument(
                                    "integration_production_service_secret")),
      []() -> Settings {
        Settings settings;
        settings.task_scheduler =
            olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
        settings.network_request_handler = olp::client::
            OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();
        return settings;
      }());

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

TEST_F(TestHereAccountOauth2IntegrationOnline, RequestTokenBadAccessSecret) {
  auto badTokenEndpoint = TokenEndpoint(
      AuthenticationCredentials(
          CustomParameters::getArgument("integration_production_service_id"),
          "BAD"),
      []() -> Settings {
        Settings settings;
        settings.task_scheduler =
            olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
        settings.network_request_handler = olp::client::
            OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();
        return settings;
      }());

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

TEST_F(TestHereAccountOauth2IntegrationOnline, RequestTokenBadTokenUrl) {
  Settings badSettings;
  badSettings.token_endpoint_url = "BAD";
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

TEST_F(TestHereAccountOauth2IntegrationOnline, RequestTokenValidExpiry) {
  auto barrier = std::make_shared<std::promise<void> >();
  m_tokenEndpoint.RequestToken(
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

TEST_F(TestHereAccountOauth2IntegrationOnline,
       DISABLED_RequestTokenConcurrent) {
  std::thread threads[5];
  auto accessTokens = std::vector<std::string>();
  auto deltaSum = std::chrono::high_resolution_clock::duration::zero();
  std::mutex globalStateMutex;

  auto startTotalTime = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 5; i++) {
    threads[i] = std::thread([&]() {
      auto barrier = std::make_shared<std::promise<void> >();
      auto start = std::chrono::high_resolution_clock::now();
      m_tokenEndpoint.RequestToken(
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

TEST_F(TestHereAccountOauth2IntegrationOnline,
       DISABLED_RequestTokenConcurrentFuture) {
  std::thread threads[5];
  auto accessTokens = std::vector<std::string>();
  auto deltaSum = std::chrono::high_resolution_clock::duration::zero();
  std::mutex globalStateMutex;

  auto startTotalTime = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 5; i++) {
    threads[i] = std::thread([&]() {
      auto start = std::chrono::high_resolution_clock::now();
      auto tokenResponse = m_tokenEndpoint.RequestToken().get();
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

TEST_F(TestHereAccountOauth2IntegrationOnline, NetworkProxySettings) {
  Settings settings;
  olp::http::NetworkProxySettings proxySettings;
  proxySettings.WithHostname("foo.bar");
  proxySettings.WithPort(42);
  settings.network_proxy_settings = proxySettings;

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

TEST_F(TestHereAccountOauth2IntegrationOnline,
       AutoRefreshingTokenValidRequest) {
  testAutoRefreshingTokenValidRequest(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken) {
        return getTokenFromSyncRequest(autoToken);
      });

  testAutoRefreshingTokenValidRequest(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken) {
        return getTokenFromAsyncRequest(autoToken);
      });
}

void testAutoRefreshingTokenInvalidRequest(
    std::function<
        TokenEndpoint::TokenResponse(const AutoRefreshingToken& autoToken)>
        func) {
  auto badTokenEndpoint =
      TokenEndpoint(AuthenticationCredentials("BAD", "BAD"), []() -> Settings {
        Settings settings;
        settings.task_scheduler =
            olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
        settings.network_request_handler = olp::client::
            OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();
        return settings;
      }());
  auto tokenResponse = func(badTokenEndpoint.RequestAutoRefreshingToken());
  EXPECT_TRUE(tokenResponse.IsSuccessful());
  EXPECT_EQ(tokenResponse.GetResult().GetHttpStatus(), 401);
  EXPECT_GT(tokenResponse.GetResult().GetErrorResponse().code, 0u);
}

TEST_F(TestHereAccountOauth2IntegrationOnline,
       AutoRefreshingTokenInvalidRequest) {
  testAutoRefreshingTokenInvalidRequest(
      [](const AutoRefreshingToken& autoToken) {
        return getTokenFromSyncRequest(autoToken);
      });

  testAutoRefreshingTokenInvalidRequest(
      [](const AutoRefreshingToken& autoToken) {
        return getTokenFromAsyncRequest(autoToken);
      });
}

void testAutoRefreshingTokenReuseToken(
    TokenEndpoint& tokenEndpoint, std::function<TokenEndpoint::TokenResponse(
                                      const AutoRefreshingToken& autoToken)>
                                      func) {
  auto autoToken = tokenEndpoint.RequestAutoRefreshingToken();
  auto tokenResponseOne = func(autoToken);
  auto tokenResponseTwo = func(autoToken);
  EXPECT_EQ(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
  EXPECT_EQ(tokenResponseOne.GetResult().GetExpiryTime(),
            tokenResponseTwo.GetResult().GetExpiryTime());
}

TEST_F(TestHereAccountOauth2IntegrationOnline, AutoRefreshingTokenReuseToken) {
  testAutoRefreshingTokenReuseToken(m_tokenEndpoint,
                                    [](const AutoRefreshingToken& autoToken) {
                                      return getTokenFromSyncRequest(autoToken);
                                    });

  testAutoRefreshingTokenReuseToken(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken) {
        return getTokenFromAsyncRequest(autoToken);
      });
}

void testAutoRefreshingTokenForceRefresh(
    TokenEndpoint& tokenEndpoint,
    std::function<TokenEndpoint::TokenResponse(
        const AutoRefreshingToken& autoToken,
        const std::chrono::seconds minimumValidity)>
        func) {
  auto autoToken = tokenEndpoint.RequestAutoRefreshingToken();
  auto tokenResponseOne = func(autoToken, std::chrono::minutes(5));
  auto tokenResponseTwo = func(autoToken, kForceRefresh);

  EXPECT_NE(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
}

TEST_F(TestHereAccountOauth2IntegrationOnline,
       AutoRefreshingTokenForceRefresh) {
  testAutoRefreshingTokenForceRefresh(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken,
                          const std::chrono::seconds minimumValidity) {
        return getTokenFromSyncRequest(autoToken, minimumValidity);
      });

  testAutoRefreshingTokenForceRefresh(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken,
                          const std::chrono::seconds minimumValidity) {
        return getTokenFromAsyncRequest(autoToken, minimumValidity);
      });
}

void testAutoRefreshingTokenExpiresInRefresh(
    TokenEndpoint& tokenEndpoint, std::function<TokenEndpoint::TokenResponse(
                                      const AutoRefreshingToken& autoToken)>
                                      func) {
  auto autoToken = tokenEndpoint.RequestAutoRefreshingToken(
      TokenRequest{std::chrono::seconds(302)});
  auto tokenResponseOne = func(autoToken);
  std::this_thread::sleep_for(std::chrono::seconds(4));
  auto tokenResponseTwo = func(autoToken);

  EXPECT_NE(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
  EXPECT_NE(tokenResponseOne.GetResult().GetExpiryTime(),
            tokenResponseTwo.GetResult().GetExpiryTime());
}

TEST_F(TestHereAccountOauth2IntegrationOnline,
       AutoRefreshingTokenExpiresInRefreshSync) {
  testAutoRefreshingTokenExpiresInRefresh(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken) {
        return getTokenFromSyncRequest(autoToken);
      });
}

TEST_F(TestHereAccountOauth2IntegrationOnline,
       AutoRefreshingTokenExpiresInRefreshAsync) {
  testAutoRefreshingTokenExpiresInRefresh(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken) {
        return getTokenFromAsyncRequest(autoToken);
      });
}

void testAutoRefreshingTokenExpiresDoNotRefresh(
    TokenEndpoint& tokenEndpoint, std::function<TokenEndpoint::TokenResponse(
                                      const AutoRefreshingToken& autoToken)>
                                      func) {
  auto autoToken = tokenEndpoint.RequestAutoRefreshingToken(
      TokenRequest{std::chrono::seconds(305)});
  auto tokenResponseOne = func(autoToken);
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto tokenResponseTwo = func(autoToken);

  EXPECT_EQ(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
  EXPECT_EQ(tokenResponseOne.GetResult().GetExpiryTime(),
            tokenResponseTwo.GetResult().GetExpiryTime());
}

TEST_F(TestHereAccountOauth2IntegrationOnline,
       AutoRefreshingTokenExpiresDoNotRefresh) {
  testAutoRefreshingTokenExpiresDoNotRefresh(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken) {
        return getTokenFromSyncRequest(autoToken);
      });

  testAutoRefreshingTokenExpiresDoNotRefresh(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken) {
        return getTokenFromAsyncRequest(autoToken);
      });
}

void testAutoRefreshingTokenExpiresDoRefresh(
    TokenEndpoint& tokenEndpoint,
    std::function<TokenEndpoint::TokenResponse(
        const AutoRefreshingToken& autoToken,
        const std::chrono::seconds minimumValidity)>
        func) {
  auto autoToken = tokenEndpoint.RequestAutoRefreshingToken(
      TokenRequest{std::chrono::seconds(1)});  // 1 second
  auto tokenResponseOne =
      func(autoToken, std::chrono::seconds(1));  // 1 sec validity window,
                                                 // shot enough to trigger
                                                 // a refresh
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto tokenResponseTwo =
      func(autoToken, std::chrono::seconds(1));  // 1 sec validity window,
                                                 // shot enough to trigger
                                                 // a refresh

  EXPECT_NE(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
  EXPECT_NE(tokenResponseOne.GetResult().GetExpiryTime(),
            tokenResponseTwo.GetResult().GetExpiryTime());
}

TEST_F(TestHereAccountOauth2IntegrationOnline,
       AutoRefreshingTokenExpiresDoRefresh) {
  testAutoRefreshingTokenExpiresDoRefresh(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken,
                          const std::chrono::seconds minimumValidity) {
        return getTokenFromSyncRequest(autoToken, minimumValidity);
      });

  testAutoRefreshingTokenExpiresDoRefresh(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken,
                          const std::chrono::seconds minimumValidity) {
        return getTokenFromAsyncRequest(autoToken, minimumValidity);
      });
}

void testAutoRefreshingTokenExpiresInAnHour(
    TokenEndpoint& tokenEndpoint,
    std::function<TokenEndpoint::TokenResponse(
        const AutoRefreshingToken& autoToken,
        const std::chrono::seconds minimumValidity)>
        func) {
  auto autoToken = tokenEndpoint.RequestAutoRefreshingToken(
      TokenRequest{std::chrono::hours(1)});
  auto tokenResponseOne = func(autoToken, std::chrono::seconds(1));
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto tokenResponseTwo = func(autoToken, std::chrono::seconds(1));

  EXPECT_EQ(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
  EXPECT_EQ(tokenResponseOne.GetResult().GetExpiryTime(),
            tokenResponseTwo.GetResult().GetExpiryTime());
}

TEST_F(TestHereAccountOauth2IntegrationOnline,
       AutoRefreshingTokenExpiresInAnHour) {
  testAutoRefreshingTokenExpiresInAnHour(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken,
                          const std::chrono::seconds minimumValidity) {
        return getTokenFromSyncRequest(autoToken, minimumValidity);
      });

  testAutoRefreshingTokenExpiresInAnHour(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken,
                          const std::chrono::seconds minimumValidity) {
        return getTokenFromAsyncRequest(autoToken, minimumValidity);
      });
}

void testAutoRefreshingTokenExpiresInASecond(
    TokenEndpoint& tokenEndpoint,
    std::function<TokenEndpoint::TokenResponse(
        const AutoRefreshingToken& autoToken,
        const std::chrono::seconds minimumValidity)>
        func) {
  auto autoToken = tokenEndpoint.RequestAutoRefreshingToken(
      TokenRequest{std::chrono::seconds(1)});
  auto tokenResponseOne = func(autoToken, std::chrono::seconds(1));
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto tokenResponseTwo = func(autoToken, std::chrono::seconds(1));

  EXPECT_NE(tokenResponseOne.GetResult().GetAccessToken(),
            tokenResponseTwo.GetResult().GetAccessToken());
  EXPECT_NE(tokenResponseOne.GetResult().GetExpiryTime(),
            tokenResponseTwo.GetResult().GetExpiryTime());
}

TEST_F(TestHereAccountOauth2IntegrationOnline,
       AutoRefreshingTokenExpiresInASecond) {
  testAutoRefreshingTokenExpiresInASecond(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken,
                          const std::chrono::seconds minimumValidity) {
        return getTokenFromSyncRequest(autoToken, minimumValidity);
      });

  testAutoRefreshingTokenExpiresInASecond(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken,
                          const std::chrono::seconds minimumValidity) {
        return getTokenFromAsyncRequest(autoToken, minimumValidity);
      });
}

void testAutoRefreshingTokenMultiThread(
    TokenEndpoint& tokenEndpoint, std::function<TokenEndpoint::TokenResponse(
                                      const AutoRefreshingToken& autoToken)>
                                      func) {
  auto autoToken = tokenEndpoint.RequestAutoRefreshingToken();

  std::thread threads[5];
  auto tokenResponses = std::vector<TokenEndpoint::TokenResponse>();
  std::mutex tokenResponsesMutex;

  for (int i = 0; i < 5; i++) {
    threads[i] = std::thread([&]() {
      std::lock_guard<std::mutex> guard(tokenResponsesMutex);
      tokenResponses.emplace_back(func(autoToken));
    });
  }

  for (int i = 0; i < 5; i++) {
    threads[i].join();
  }

  ASSERT_EQ(tokenResponses.size(), 5u);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(tokenResponses[i].GetResult().GetAccessToken(),
              tokenResponses[i + 1].GetResult().GetAccessToken());
    EXPECT_EQ(tokenResponses[i].GetResult().GetExpiryTime(),
              tokenResponses[i + 1].GetResult().GetExpiryTime());
  }
}

TEST_F(TestHereAccountOauth2IntegrationOnline, AutoRefreshingTokenMultiThread) {
  testAutoRefreshingTokenMultiThread(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken) {
        return getTokenFromSyncRequest(autoToken);
      });

  testAutoRefreshingTokenMultiThread(
      m_tokenEndpoint, [](const AutoRefreshingToken& autoToken) {
        return getTokenFromAsyncRequest(autoToken);
      });
}
