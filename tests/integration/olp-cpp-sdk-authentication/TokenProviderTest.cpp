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

#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/NetworkMock.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/porting/warning_disable.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
#include "../olp-cpp-sdk-dataservice-read/HttpResponses.h"
#include "AuthenticationMockedResponses.h"

namespace http = olp::http;
namespace client = olp::client;
namespace authentication = olp::authentication;
namespace dataservice = olp::dataservice;
using testing::_;
using testing::AnyOf;
using testing::Not;

namespace {

// Catalog defines
static constexpr auto kCatalog =
    "hrn:here:data::olp-here-test:here-optimized-map-for-visualization-2";
static constexpr int64_t KVersion = 108;
static constexpr auto kLayer = "testlayer";
static constexpr auto kPartition = "269";
constexpr auto kWaitTimeout = std::chrono::seconds(3);
constexpr auto kMaxRetryAttempts = 5;
constexpr auto kMinTimeout = 1;
constexpr auto kRequestId = 42;

// Request defines
static const std::string kTimestampUrl =
    R"(https://account.api.here.com/timestamp)";

static const std::string kOAuthTokenUrl =
    R"(https://account.api.here.com/oauth2/token)";

// Response defines
constexpr char kHttpResponseLookupQuery[] =
    R"jsonString([{"api":"query","version":"v1","baseURL":"https://query.data.api.platform.here.com/query/v1/catalogs/here-optimized-map-for-visualization-2","parameters":{}}])jsonString";

constexpr char kHttpResponsePartition_269[] =
    R"jsonString({ "partitions": [{"version":4,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"}]})jsonString";

constexpr char kHttpResponseLookupBlob[] =
    R"jsonString([{"api":"blob","version":"v1","baseURL":"https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/here-optimized-map-for-visualization-2","parameters":{}}])jsonString";

constexpr char kHttpResponseBlobData_269[] = R"jsonString(DT_2_0031)jsonString";

constexpr char kResponseTime[] = R"JSON({"timestamp":123})JSON";

constexpr char kResponseValidJson[] = R"JSON(
   {"accessToken":"tyJhbGciOiJSUzUxMiIsImN0eSI6IkpXVCIsImlzcyI6IkhFUkUiLCJhaWQiOiJTcFR5dkQ0RjZ1dWhVY0t3ZjBPRCIsImlhdCI6MTUyMjY5OTY2MywiZXhwIjoxNTIyNzAzMjYzLCJraWQiOiJqMSJ9.ZXlKaGJHY2lPaUprYVhJaUxDSmxibU1pT2lKQk1qVTJRMEpETFVoVE5URXlJbjAuLkNuSXBWVG14bFBUTFhqdFl0ODVodVEuTk1aMzRVSndtVnNOX21Zd3pwa1UydVFfMklCbE9QeWw0VEJWQnZXczcwRXdoQWRld0tpR09KOGFHOWtKeTBoYWg2SS03Y01WbXQ4S3ppUHVKOXZqV2U1Q0F4cER0LU0yQUxhQTJnZWlIZXJuaEEwZ1ZRR3pVakw5OEhDdkpEc2YuQXhxNTRPTG9FVDhqV2ZreTgtZHY4ZUR1SzctRnJOWklGSms0RHZGa2F5Yw.bfSc5sXovW0-yGTqWDZtsVvqIxeNl9IGFbtzRBRkHCHEjthZzeRscB6oc707JTpiuRmDKJe6oFU03RocTS99YBlM3p5rP2moadDNmP3Uag4elo6z0ZE_w1BP7So7rMX1k4NymfEATdmyXVnjAhBlTPQqOYIWV-UNCXWCIzLSuwaJ96N1d8XZeiA1jkpsp4CKfcSSm9hgsKNA95SWPnZAHyqOYlO0sDE28osOIjN2UVSUKlO1BDtLiPLta_dIqvqFUU5aRi_dcYqkJcZh195ojzeAcvDGI6HqS2zUMTdpYUhlwwfpkxGwrFmlAxgx58xKSeVt0sPvtabZBAW8uh2NGg","tokenType":"bearer","expiresIn":3599}
    )JSON";

static const std::string kResponseToken =
    "tyJhbGciOiJSUzUxMiIsImN0eSI6IkpXVCIsImlzcyI6IkhFUkUiLCJhaWQiOiJTcFR5dkQ0Rj"
    "Z1dWhVY0t3ZjBPRCIsImlhdCI6MTUyMjY5OTY2MywiZXhwIjoxNTIyNzAzMjYzLCJraWQiOiJq"
    "MSJ9."
    "ZXlKaGJHY2lPaUprYVhJaUxDSmxibU1pT2lKQk1qVTJRMEpETFVoVE5URXlJbjAuLkNuSXBWVG"
    "14bFBUTFhqdFl0ODVodVEuTk1aMzRVSndtVnNOX21Zd3pwa1UydVFfMklCbE9QeWw0VEJWQnZX"
    "czcwRXdoQWRld0tpR09KOGFHOWtKeTBoYWg2SS03Y01WbXQ4S3ppUHVKOXZqV2U1Q0F4cER0LU"
    "0yQUxhQTJnZWlIZXJuaEEwZ1ZRR3pVakw5OEhDdkpEc2YuQXhxNTRPTG9FVDhqV2ZreTgtZHY4"
    "ZUR1SzctRnJOWklGSms0RHZGa2F5Yw.bfSc5sXovW0-"
    "yGTqWDZtsVvqIxeNl9IGFbtzRBRkHCHEjthZzeRscB6oc707JTpiuRmDKJe6oFU03RocTS99YB"
    "lM3p5rP2moadDNmP3Uag4elo6z0ZE_w1BP7So7rMX1k4NymfEATdmyXVnjAhBlTPQqOYIWV-"
    "UNCXWCIzLSuwaJ96N1d8XZeiA1jkpsp4CKfcSSm9hgsKNA95SWPnZAHyqOYlO0sDE28osOIjN2"
    "UVSUKlO1BDtLiPLta_dIqvqFUU5aRi_"
    "dcYqkJcZh195ojzeAcvDGI6HqS2zUMTdpYUhlwwfpkxGwrFmlAxgx58xKSeVt0sPvtabZBAW8u"
    "h2NGg";

static const std::string kResponseTooManyRequests =
    R"JSON({"errorCode":429002,"message":"Request blocked because too many requests were made. Please wait for a while before making a new request."})JSON";

http::NetworkResponse GetResponse(int status) {
  return http::NetworkResponse().WithStatus(status);
}

class TokenProviderTest : public ::testing::Test {
 public:
  TokenProviderTest() = default;

  void SetUp() override {
    network_mock_ = std::make_shared<NetworkMock>();
    settings_.network_request_handler = network_mock_;
    settings_.task_scheduler =
        client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
  }

  void TearDown() override {
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
    network_mock_.reset();
    settings_.task_scheduler.reset();
  }

  template <long long MinimumValidity>
  client::OlpClientSettings GetSettings(bool use_system_time = false) const {
    client::AuthenticationSettings auth_settings;
    authentication::Settings token_provider_settings(
        {"fake.key.id", "fake.key.secret"});
    token_provider_settings.task_scheduler = settings_.task_scheduler;
    token_provider_settings.network_request_handler =
        settings_.network_request_handler;
    token_provider_settings.use_system_time = use_system_time;
    auth_settings.token_provider =
        authentication::TokenProvider<MinimumValidity>(token_provider_settings);

    client::OlpClientSettings settings = settings_;
    settings.authentication_settings = auth_settings;
    return settings;
  }

  client::OlpClientSettings settings_;
  std::shared_ptr<NetworkMock> network_mock_;
};

TEST_F(TokenProviderTest, SingleTokenMultipleUsers) {
  constexpr size_t kCount = 3u;
  auto settings = GetSettings<authentication::kDefaultMinimumValidity>();
  auto catalog = client::HRN::FromString(kCatalog);

  {
    SCOPED_TRACE("Request token once");

    // Setup once expects for the token. All clients need to use the same token.
    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kResponseTime))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kResponseValidJson));

    ASSERT_TRUE(settings.authentication_settings);
    ASSERT_TRUE(settings.authentication_settings->token_provider);

    client::CancellationContext context;
    const auto token_response =
        settings.authentication_settings->token_provider(context);
    ASSERT_TRUE(token_response);
    EXPECT_EQ(token_response.GetResult().GetAccessToken(), kResponseToken);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }

  {
    SCOPED_TRACE("Multiple requests, multiple clients");

    // Create test layer clients, all using the same token provider
    for (size_t index = 0; index < kCount; ++index) {
      EXPECT_CALL(*network_mock_, Send(AnyOf(IsGetRequest(kTimestampUrl),
                                             IsPostRequest(kOAuthTokenUrl)),
                                       _, _, _, _))
          .Times(0);

      EXPECT_CALL(*network_mock_,
                  Send(Not(AnyOf(IsGetRequest(kTimestampUrl),
                                 IsPostRequest(kOAuthTokenUrl))),
                       _, _, _, _))
          .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                       kHttpResponseLookupQuery))
          .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                       kHttpResponsePartition_269))
          .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                       kHttpResponseLookupBlob))
          .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                       kHttpResponseBlobData_269));

      auto client = std::make_unique<dataservice::read::VersionedLayerClient>(
          catalog, kLayer, KVersion, settings);

      auto cancellation_future = client->GetData(
          dataservice::read::DataRequest().WithPartitionId(kPartition));

      auto future = cancellation_future.GetFuture();
      ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
      auto response = future.get();

      ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
      ASSERT_NE(response.GetResult(), nullptr);
      ASSERT_FALSE(response.GetResult()->empty());

      // Verify token is still the same
      client::CancellationContext context;
      const auto token_response =
          settings.authentication_settings->token_provider(context);
      ASSERT_TRUE(token_response);
      EXPECT_EQ(token_response.GetResult().GetAccessToken(), kResponseToken);
    }
  }

  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(TokenProviderTest, UseLocalAndServerTime) {
  {
    SCOPED_TRACE("Request token, use system time");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(kTimestampUrl), _, _, _, _))
        .Times(0);

    EXPECT_CALL(*network_mock_, Send(IsPostRequest(kOAuthTokenUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kResponseValidJson));
    auto settings = GetSettings<authentication::kDefaultMinimumValidity>(true);
    ASSERT_TRUE(settings.authentication_settings);
    ASSERT_TRUE(settings.authentication_settings->token_provider);

    client::CancellationContext context;
    const auto token_response =
        settings.authentication_settings->token_provider(context);
    ASSERT_TRUE(token_response);
    EXPECT_EQ(token_response.GetResult().GetAccessToken(), kResponseToken);
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }

  {
    SCOPED_TRACE("Request token, use server time");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(kTimestampUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kResponseTime));

    EXPECT_CALL(*network_mock_, Send(IsPostRequest(kOAuthTokenUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kResponseValidJson));
    auto settings = GetSettings<authentication::kDefaultMinimumValidity>(false);
    ASSERT_TRUE(settings.authentication_settings);
    ASSERT_TRUE(settings.authentication_settings->token_provider);

    client::CancellationContext context;
    const auto token_response =
        settings.authentication_settings->token_provider(context);
    ASSERT_TRUE(token_response);
    EXPECT_EQ(token_response.GetResult().GetAccessToken(), kResponseToken);
  }
  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(TokenProviderTest, ConcurrentRequests) {
  EXPECT_CALL(*network_mock_, Send(IsPostRequest(kOAuthTokenUrl), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kResponseValidJson));
  auto settings = GetSettings<authentication::kDefaultMinimumValidity>(true);
  ASSERT_TRUE(settings.authentication_settings);
  ASSERT_TRUE(settings.authentication_settings->token_provider);

  const auto kRequestCount = 5;
  std::vector<std::thread> threads;
  std::vector<std::future<client::OauthTokenResponse>> futures;
  for (auto i = 0; i < kRequestCount; ++i) {
    auto promise = std::make_shared<std::promise<client::OauthTokenResponse>>();
    threads.emplace_back([promise, settings]() {
      client::CancellationContext context;
      promise->set_value(
          settings.authentication_settings->token_provider(context));
    });
    futures.emplace_back(promise->get_future());
  }

  for (auto i = 0; i < kRequestCount; ++i) {
    threads[i].join();
    auto token_response = futures[i].get();
    ASSERT_TRUE(token_response);
    EXPECT_EQ(token_response.GetResult().GetAccessToken(), kResponseToken);
  }

  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(TokenProviderTest, RetrySettings) {
  authentication::Settings token_provider_settings(
      {"fake.key.id", "fake.key.secret"});
  token_provider_settings.task_scheduler = settings_.task_scheduler;
  token_provider_settings.network_request_handler =
      settings_.network_request_handler;
  token_provider_settings.use_system_time = true;
  token_provider_settings.retry_settings.max_attempts = kMaxRetryAttempts;
  token_provider_settings.retry_settings.timeout = kMinTimeout;

  const auto retry_predicate = testing::Property(
      &http::NetworkRequest::GetSettings,
      testing::AllOf(
          testing::Property(&http::NetworkSettings::GetConnectionTimeout,
                            kMinTimeout),
          testing::Property(&http::NetworkSettings::GetTransferTimeout,
                            kMinTimeout)));
  {
    SCOPED_TRACE("Max attempts");

    EXPECT_CALL(*network_mock_, Send(retry_predicate, _, _, _, _))
        .Times(kMaxRetryAttempts)
        .WillRepeatedly(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS)
                .WithError("Too many requests"),
            kResponseTooManyRequests));

    const authentication::TokenProviderDefault token_provider(
        token_provider_settings);

    client::CancellationContext context;
    const auto token = token_provider(context);
    ASSERT_FALSE(token);
    EXPECT_EQ(token.GetError().GetHttpStatusCode(),
              http::HttpStatusCode::TOO_MANY_REQUESTS);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }

  {
    SCOPED_TRACE("Timeout");

    std::future<void> async_finish_future;
    client::CancellationContext context;

    EXPECT_CALL(*network_mock_, Send(IsPostRequest(kOAuthTokenUrl), _, _, _, _))
        .WillOnce(
            testing::WithArg<2>([&](http::Network::Callback callback) {
              async_finish_future = std::async(std::launch::async, [=]() {
                // Oversleep the timeout period.
                std::this_thread::sleep_for(
                    std::chrono::seconds(kMinTimeout * 2));

                callback(http::NetworkResponse()
                             .WithStatus(http::HttpStatusCode::OK)
                             .WithRequestId(kRequestId));
              });

              return http::SendOutcome(kRequestId);
            }));

    EXPECT_CALL(*network_mock_, Cancel(kRequestId)).Times(1);

    const authentication::TokenProviderDefault token_provider(
        token_provider_settings);
    const auto token_response = token_provider(context);

    ASSERT_EQ(async_finish_future.wait_for(kWaitTimeout),
              std::future_status::ready);
    ASSERT_FALSE(token_response);
    EXPECT_EQ(token_response.GetError().GetHttpStatusCode(),
              static_cast<int>(http::ErrorCode::TIMEOUT_ERROR));

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(TokenProviderTest, CancellableProvider) {
  authentication::Settings token_provider_settings(
      {"fake.key.id", "fake.key.secret"});
  token_provider_settings.task_scheduler = settings_.task_scheduler;
  token_provider_settings.network_request_handler =
      settings_.network_request_handler;
  token_provider_settings.use_system_time = true;
  token_provider_settings.retry_settings.max_attempts = 1;  // Disable retries

  {
    SCOPED_TRACE("TokenResult contains token");

    const int status_code = http::HttpStatusCode::OK;

    EXPECT_CALL(*network_mock_, Send(IsPostRequest(kOAuthTokenUrl), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(GetResponse(status_code), kResponseValidJson));

    const authentication::TokenProviderDefault token_provider(
        token_provider_settings);

    client::CancellationContext context;
    const auto token_response = token_provider(context);
    ASSERT_TRUE(token_response);
    EXPECT_EQ(token_response.GetResult().GetAccessToken(), kResponseToken);

    EXPECT_TRUE(token_provider);
    EXPECT_EQ(token_provider.GetHttpStatusCode(), status_code);

    EXPECT_EQ(token_provider.GetErrorResponse().code, 0);

    PORTING_PUSH_WARNINGS()
    PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")
    EXPECT_EQ(token_provider(), kResponseToken);
    PORTING_POP_WARNINGS()

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }

  {
    SCOPED_TRACE("TokenResult contains error");

    const int status_code = http::HttpStatusCode::TOO_MANY_REQUESTS;

    EXPECT_CALL(*network_mock_, Send(IsPostRequest(kOAuthTokenUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            GetResponse(status_code).WithError("Too many requests"),
            kResponseTooManyRequests));

    const authentication::TokenProviderDefault token_provider(
        token_provider_settings);

    client::CancellationContext context;
    const auto token_response = token_provider(context);
    ASSERT_FALSE(token_response);
    EXPECT_EQ(token_response.GetError().GetHttpStatusCode(), status_code);
    EXPECT_EQ(token_response.GetError().GetMessage(), kResponseTooManyRequests);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }

  {
    SCOPED_TRACE("Token request cancelled");

    std::future<void> async_finish_future;
    std::promise<void> network_wait_promise;

    client::CancellationContext context;
    EXPECT_CALL(*network_mock_, Send(IsPostRequest(kOAuthTokenUrl), _, _, _, _))
        .WillOnce(
            testing::WithArg<2>([&](http::Network::Callback callback) {
              async_finish_future =
                  std::async(std::launch::async, [&, callback]() {
                    std::this_thread::sleep_for(
                        std::chrono::seconds(kMinTimeout));
                    context.CancelOperation();

                    EXPECT_EQ(network_wait_promise.get_future().wait_for(
                                  kWaitTimeout),
                              std::future_status::ready);

                    callback(http::NetworkResponse()
                                 .WithStatus(http::HttpStatusCode::OK)
                                 .WithRequestId(kRequestId));
                  });

              return http::SendOutcome(kRequestId);
            }));

    EXPECT_CALL(*network_mock_, Cancel(kRequestId)).Times(1);

    const authentication::TokenProviderDefault token_provider(
        token_provider_settings);
    const auto token_response = token_provider(context);
    network_wait_promise.set_value();

    ASSERT_EQ(async_finish_future.wait_for(kWaitTimeout),
              std::future_status::ready);
    ASSERT_FALSE(token_response);
    EXPECT_EQ(token_response.GetError().GetHttpStatusCode(),
              static_cast<int>(http::ErrorCode::CANCELLED_ERROR));

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }

  {
    SCOPED_TRACE("TokenResponse is not successful");

    // Disables the network to get an error.
    token_provider_settings.network_request_handler = nullptr;

    const authentication::TokenProviderDefault token_provider(
        token_provider_settings);

    client::CancellationContext context;
    const auto token_response = token_provider(context);
    ASSERT_FALSE(token_response);
    EXPECT_EQ(token_response.GetError().GetErrorCode(),
              client::ErrorCode::NetworkConnection);
    EXPECT_EQ(token_response.GetError().GetMessage(),
              "Cannot sign in while offline");

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

}  // namespace
