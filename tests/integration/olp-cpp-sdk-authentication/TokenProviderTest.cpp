/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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
#include <olp/dataservice/read/VersionedLayerClient.h>
#include "../olp-cpp-sdk-dataservice-read/HttpResponses.h"

using namespace olp;
using namespace olp::tests::common;
using namespace testing;

namespace {

// Catalog defines
static constexpr auto kCatalog =
    "hrn:here:data::olp-here-test:here-optimized-map-for-visualization-2";
static constexpr int64_t KVersion = 108;
static constexpr auto kLayer = "testlayer";
static constexpr auto kPartition = "269";
constexpr auto kWaitTimeout = std::chrono::seconds(3);

// Request defines
static const std::string kTimestampUrl =
    R"( https://account.api.here.com/timestamp)";

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

http::NetworkResponse GetResponse(int status) {
  return http::NetworkResponse().WithStatus(status);
}

class TokenProviderTest : public ::testing::Test {
 public:
  TokenProviderTest()
      : token_provider_settings_({"fake.key.id", "fake.key.secret"}) {}

  void SetUp() override {
    network_mock_ = std::make_shared<StrictMock<NetworkMock>>();

    settings_.network_request_handler = network_mock_;
    settings_.task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

    token_provider_settings_.task_scheduler = settings_.task_scheduler;
    token_provider_settings_.network_request_handler =
        settings_.network_request_handler;
  }

  void TearDown() override {
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
    network_mock_.reset();
    settings_.task_scheduler.reset();
  }

  template <long long MinimumValidity>
  client::OlpClientSettings GetSettings() const {
    olp::client::AuthenticationSettings auth_settings;
    auth_settings.provider =
        olp::authentication::TokenProvider<MinimumValidity>(
            token_provider_settings_);

    olp::client::OlpClientSettings settings = settings_;
    settings.authentication_settings = auth_settings;
    return settings;
  }

  olp::client::OlpClientSettings settings_;
  olp::authentication::Settings token_provider_settings_;
  std::shared_ptr<StrictMock<NetworkMock>> network_mock_;
};

TEST_F(TokenProviderTest, SingleTokenMultipleUsers) {
  constexpr size_t kCount = 3u;
  auto settings = GetSettings<authentication::kDefaultMinimumValidity>();
  auto catalog = olp::client::HRN::FromString(kCatalog);

  {
    SCOPED_TRACE("Request token once");

    // Setup once expects for the token. All clients need to use the same token.
    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kResponseTime))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kResponseValidJson));

    ASSERT_TRUE(settings.authentication_settings);
    ASSERT_TRUE(settings.authentication_settings->provider);

    auto token = settings.authentication_settings->provider();
    EXPECT_EQ(token, kResponseToken);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }

  {
    SCOPED_TRACE("Multiple requests, multiple clients");

    // Create test layer clients, all using the same token provider
    for (size_t index = 0; index < kCount; ++index) {
      EXPECT_CALL(*network_mock_, Send(Not(AnyOf(IsGetRequest(kTimestampUrl),
                                                 IsGetRequest(kOAuthTokenUrl))),
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
      auto token = settings.authentication_settings->provider();
      EXPECT_EQ(token, kResponseToken);
    }
  }

  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
}

}  // namespace
