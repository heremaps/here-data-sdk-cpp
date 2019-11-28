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
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include "ApiClientLookup.h"

namespace {

using namespace testing;
using namespace olp::dataservice::write;
using namespace olp::tests::common;

const std::string kCatalog = "hrn:here:data:::some_test_catalog";

const std::string kConfigServiceName = "config";
const std::string kPublishServiceName = "publish";

const std::string kConfigRequestUrl =
    "https://api-lookup.data.api.platform.here.com/lookup/v1/"
    "platform/apis/config/v1";
const std::string kPublishRequestUrl =
    "https://api-lookup.data.api.platform.here.com/lookup/v1/"
    "resources/" +
    kCatalog + "/apis/publish/v1";

const std::string kConfigBaseUrl =
    "https://config.data.api.platform.here.com/config/v1";
const std::string kPublishBaseUrl =
    "https://publish.data.api.platform.here.com/publish/v1/"
    "catalogs/" +
    kCatalog;

const std::string kConfigHttpResponse =
    R"jsonString([{"api":"config","version":"v1","baseURL":")jsonString" +
    kConfigBaseUrl + R"jsongString(","parameters":{}}])jsongString";

const std::string kPublishHttpResponse =
    R"jsonString([{"api":"publish","version":"v1","baseURL":")jsonString" +
    kPublishBaseUrl + R"jsonString(","parameters":{}}])jsonString";

// Enum which controls the which Lookup API will be tested
enum LookupApiType {
  // 'config' service - PlatformApi::GetApis
  Config,
  // 'publish' service - ResourcesApi::GetApis
  Resources
};

class ApiClientLookupTest : public ::testing::TestWithParam<LookupApiType> {
 protected:
  void SetUp() override {
    network_ = std::make_shared<
        testing::StrictMock<olp::tests::common::NetworkMock>>();

    settings_.network_request_handler = network_;
    settings_.retry_settings.timeout = 1;
  }

  void TearDown() override {
    network_.reset();
    auto network = std::move(settings_.network_request_handler);
    settings_ = olp::client::OlpClientSettings();
    // when test ends we must be sure that network pointer is not captured
    // anywhere
    EXPECT_EQ(network.use_count(), 1);
  }

  const std::string GetServiceName() {
    switch (GetParam()) {
      case LookupApiType::Config:
        return kConfigServiceName;
      case LookupApiType::Resources:
        return kPublishServiceName;
      default:
        assert(false);
    }
  }

  const std::string GetLookupApiRequestUrl() {
    switch (GetParam()) {
      case LookupApiType::Config:
        return kConfigRequestUrl;
      case LookupApiType::Resources:
        return kPublishRequestUrl;
      default:
        assert(false);
    }
  }

  const std::string GetLookupApiBaseUrl() {
    switch (GetParam()) {
      case LookupApiType::Config:
        return kConfigBaseUrl;
      case LookupApiType::Resources:
        return kPublishBaseUrl;
      default:
        assert(false);
    }
  }

  const std::string GetLookupApiHttpResponse() {
    switch (GetParam()) {
      case LookupApiType::Config:
        return kConfigHttpResponse;
      case LookupApiType::Resources:
        return kPublishHttpResponse;
      default:
        assert(false);
    }
  }

 protected:
  olp::client::OlpClientSettings settings_;
  std::shared_ptr<testing::StrictMock<olp::tests::common::NetworkMock>>
      network_;
};

TEST_P(ApiClientLookupTest, LookupApiClientSync) {
  const std::string kServiceName = GetServiceName();
  const std::string kServiceUrl = "http://random_service.com";
  const std::string kServiceVersion = "v1";

  const std::string kCacheKey =
      kCatalog + "::" + kServiceName + "::" + kServiceVersion + "::api";
  const auto kHRN = olp::client::HRN::FromString(kCatalog);

  {
    SCOPED_TRACE("Fetch from cache positive");
    auto cache =
        std::make_shared<testing::StrictMock<olp::tests::common::CacheMock>>();
    EXPECT_CALL(*cache, Get(kCacheKey, _))
        .Times(1)
        .WillOnce(Return(kServiceUrl));
    auto settings_with_cache = settings_;
    settings_with_cache.cache = cache;

    auto response = ApiClientLookup::LookupApiClient(
        kHRN, olp::client::CancellationContext{}, kServiceName, kServiceVersion,
        settings_with_cache);
    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kServiceUrl);
  }

  {
    SCOPED_TRACE("Fetch from cache negative and fetch from network_");
    auto cache =
        std::make_shared<testing::StrictMock<olp::tests::common::CacheMock>>();
    EXPECT_CALL(*cache, Get(kCacheKey, _)).Times(1);
    EXPECT_CALL(*cache, Put(kCacheKey, _, _, _))
        .Times(1)
        .WillOnce(Return(true));
    auto settings_with_cache = settings_;
    settings_with_cache.cache = cache;

    EXPECT_CALL(*network_,
                Send(IsGetRequest(GetLookupApiRequestUrl()), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     GetLookupApiHttpResponse()));

    auto response = ApiClientLookup::LookupApiClient(
        kHRN, olp::client::CancellationContext{}, kServiceName, kServiceVersion,
        settings_with_cache);
    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), GetLookupApiBaseUrl());
    Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Fetch from network_ without cache");
    EXPECT_CALL(*network_,
                Send(IsGetRequest(GetLookupApiRequestUrl()), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     GetLookupApiHttpResponse()));

    auto response = ApiClientLookup::LookupApiClient(
        kHRN, olp::client::CancellationContext{}, kServiceName, kServiceVersion,
        settings_);
    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), GetLookupApiBaseUrl());
    Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Network error propagated to the user");
    EXPECT_CALL(*network_,
                Send(IsGetRequest(GetLookupApiRequestUrl()), _, _, _, _))
        .Times(1)
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::UNAUTHORIZED),
                               "Failed"));

    auto response = ApiClientLookup::LookupApiClient(
        kHRN, olp::client::CancellationContext{}, kServiceName, kServiceVersion,
        settings_);
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::AccessDenied);
    Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Network request timed out");
    EXPECT_CALL(*network_,
                Send(IsGetRequest(GetLookupApiRequestUrl()), _, _, _, _))
        .Times(1)
        .WillOnce([=](olp::http::NetworkRequest request,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback header_callback,
                      olp::http::Network::DataCallback data_callback)
                      -> olp::http::SendOutcome {
          // note no network_ response thread spawns
          constexpr auto kUnusedRequestId = 42;
          return olp::http::SendOutcome(kUnusedRequestId);
        });
    EXPECT_CALL(*network_, Cancel(_)).Times(1).WillOnce(Return());

    auto response = ApiClientLookup::LookupApiClient(
        kHRN, olp::client::CancellationContext{}, kServiceName, kServiceVersion,
        settings_);
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::RequestTimeout);
    Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Network request cancelled by network_ internally");
    EXPECT_CALL(*network_,
                Send(IsGetRequest(GetLookupApiRequestUrl()), _, _, _, _))
        .Times(1)
        .WillOnce([=](olp::http::NetworkRequest request,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback header_callback,
                      olp::http::Network::DataCallback data_callback)
                      -> olp::http::SendOutcome {
          return olp::http::SendOutcome(olp::http::ErrorCode::CANCELLED_ERROR);
        });

    auto response = ApiClientLookup::LookupApiClient(
        kHRN, olp::client::CancellationContext{}, kServiceName, kServiceVersion,
        settings_);
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    olp::client::CancellationContext context;
    SCOPED_TRACE("Network request cancelled by user");
    EXPECT_CALL(*network_,
                Send(IsGetRequest(GetLookupApiRequestUrl()), _, _, _, _))
        .Times(1)
        .WillOnce([=](olp::http::NetworkRequest request,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback header_callback,
                      olp::http::Network::DataCallback data_callback) mutable
                  -> olp::http::SendOutcome {
          std::thread([context]() mutable { context.CancelOperation(); })
              .detach();
          constexpr auto kUnusedRequestId = 42;
          return olp::http::SendOutcome(kUnusedRequestId);
        });
    EXPECT_CALL(*network_, Cancel(_)).Times(1).WillOnce(Return());
    auto response = ApiClientLookup::LookupApiClient(
        kHRN, context, kServiceName, kServiceVersion, settings_);
    EXPECT_FALSE(response.IsSuccessful());
    auto err_code = response.GetError().GetErrorCode();
    EXPECT_EQ(err_code, olp::client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Network request cancelled before execution setup");
    olp::client::CancellationContext context;
    context.CancelOperation();
    auto response = ApiClientLookup::LookupApiClient(
        kHRN, context, kServiceName, kServiceVersion, settings_);

    EXPECT_FALSE(response.IsSuccessful());
    auto err_code = response.GetError().GetErrorCode();
    EXPECT_EQ(err_code, olp::client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Pass HRN with bad catalog");

    const auto kBadHrn = olp::client::HRN::FromString("hrn:wrong:data:catalog");

    auto response = ApiClientLookup::LookupApiClient(
        kBadHrn, olp::client::CancellationContext{}, kServiceName,
        kServiceVersion, settings_);

    EXPECT_FALSE(response.IsSuccessful());
    auto err_code = response.GetError().GetErrorCode();
    EXPECT_EQ(err_code, olp::client::ErrorCode::NotFound);
    Mock::VerifyAndClearExpectations(network_.get());
  }
}

INSTANTIATE_TEST_SUITE_P(, ApiClientLookupTest,
                         ::testing::Values(LookupApiType::Config,
                                           LookupApiType::Resources));

}  // namespace
