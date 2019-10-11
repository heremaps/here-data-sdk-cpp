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

#include "repositories/CatalogRepository.h"

#include <gtest/gtest.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include <olp/core/client/OlpClientFactory.h>
#include "ApiClientLookup.h"

#define OLP_SDK_URL_LOOKUP_METADATA \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data:::hereos-internal-test-v2/apis/metadata/v1)"
#define OLP_SDK_HTTP_RESPONSE_LOOKUP_METADATA \
  R"jsonString([{"api":"metadata","version":"v1","baseURL":"https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString"

#define OLP_SDK_URL_LATEST_CATALOG_VERSION \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/versions/latest?startVersion=-1)"

#define OLP_SDK_HTTP_RESPONSE_LATEST_CATALOG_VERSION \
  R"jsonString({"version":4})jsonString"

namespace {

using namespace olp::dataservice::read;
using namespace ::testing;
using namespace olp::tests::common;

const std::string kCatalog = "hrn:here:data:::hereos-internal-test-v2";
const std::string kCacheKey = kCatalog + "::latestVersion";
const std::string kServiceName = "metadata";
const std::string kServiceVersion = "v1";
const std::string kCacheKeyMetadata =
    kCatalog + "::" + kServiceName + "::" + kServiceVersion + "::api";
const std::string kLookupUrl =
    "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/" +
    kCatalog + "/apis/" + kServiceName + "/" + kServiceVersion;
const auto kHRN = olp::client::HRN::FromString(kCatalog);

class CatalogRepositoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    cache_ = std::make_shared<NiceMock<CacheMock>>();
    network_ = std::make_shared<NiceMock<NetworkMock>>();

    settings_.network_request_handler = network_;
    settings_.cache = cache_;
  }

  void TearDown() override {
    settings_.network_request_handler.reset();
    settings_.cache.reset();

    network_.reset();
    cache_.reset();
  }

  std::shared_ptr<CacheMock> cache_;
  std::shared_ptr<NetworkMock> network_;
  olp::client::OlpClientSettings settings_;
};

TEST_F(CatalogRepositoryTest, GetLatestVersionCacheOnlyFound) {
  olp::client::CancellationContext context;

  auto request = DataRequest();

  request.WithFetchOption(CacheOnly);

  model::VersionResponse cached_vesrion;
  cached_vesrion.SetVersion(10);

  EXPECT_CALL(*cache_, Get(kCacheKey, _))
      .Times(1)
      .WillOnce(Return(cached_vesrion));

  auto response = repository::CatalogRepository::GetLatestVersion(
      kHRN, context, request, settings_);

  ASSERT_TRUE(response.IsSuccessful());
  EXPECT_EQ(10, response.GetResult().GetVersion());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionCacheOnlyNotFound) {
  olp::client::CancellationContext context;

  auto request = olp::dataservice::read::DataRequest();

  request.WithFetchOption(CacheOnly);

  EXPECT_CALL(*cache_, Get(_, _)).Times(1).WillOnce(Return(boost::any{}));

  ON_CALL(*network_, Send(_, _, _, _, _))
      .WillByDefault([](olp::http::NetworkRequest, olp::http::Network::Payload,
                        olp::http::Network::Callback,
                        olp::http::Network::HeaderCallback,
                        olp::http::Network::DataCallback) {
        ADD_FAILURE() << "Should not be called with CacheOnly";
        return olp::http::SendOutcome(olp::http::ErrorCode::UNKNOWN_ERROR);
      });

  auto response =
      olp::dataservice::read::repository::CatalogRepository::GetLatestVersion(
          kHRN, context, request, settings_);

  EXPECT_FALSE(response.IsSuccessful());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionOnlineOnlyNotFound) {
  olp::client::CancellationContext context;

  auto request = olp::dataservice::read::DataRequest();

  request.WithFetchOption(OnlineOnly);

  ON_CALL(*cache_, Get(_, _))
      .WillByDefault([](const std::string&, const olp::cache::Decoder&) {
        ADD_FAILURE() << "Cache should not be used in OnlineOnly request";
        return boost::any{};
      });

  EXPECT_CALL(*network_,
              Send(IsGetRequest(OLP_SDK_URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1)
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::NOT_FOUND),
          ""));

  auto response =
      olp::dataservice::read::repository::CatalogRepository::GetLatestVersion(
          kHRN, context, request, settings_);

  EXPECT_FALSE(response.IsSuccessful());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionOnlineOnlyFoundAndCacheWritten) {
  olp::client::CancellationContext context;

  auto request = olp::dataservice::read::DataRequest();

  request.WithFetchOption(OnlineOnly);

  ON_CALL(*cache_, Get(_, _))
      .WillByDefault([](const std::string&, const olp::cache::Decoder&) {
        ADD_FAILURE() << "Cache should not be used in OnlineOnly request";
        return boost::any{};
      });

  EXPECT_CALL(*cache_, Put(Eq(kCacheKey), _, _, _)).Times(1);

  EXPECT_CALL(*cache_, Put(Eq(kCacheKeyMetadata), _, _, _)).Times(1);

  EXPECT_CALL(*network_,
              Send(IsGetRequest(OLP_SDK_URL_LOOKUP_METADATA), _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          OLP_SDK_HTTP_RESPONSE_LOOKUP_METADATA));

  EXPECT_CALL(*network_, Send(IsGetRequest(OLP_SDK_URL_LATEST_CATALOG_VERSION),
                              _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          OLP_SDK_HTTP_RESPONSE_LATEST_CATALOG_VERSION));

  auto response =
      olp::dataservice::read::repository::CatalogRepository::GetLatestVersion(
          kHRN, context, request, settings_);

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_EQ(4, response.GetResult().GetVersion());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionOnlineOnlyUserCancelled1) {
  olp::client::CancellationContext context;

  auto request = olp::dataservice::read::DataRequest();

  std::promise<bool> cancelled;

  ON_CALL(*network_,
          Send(IsGetRequest(OLP_SDK_URL_LOOKUP_METADATA), _, _, _, _))
      .WillByDefault([&context](olp::http::NetworkRequest,
                                olp::http::Network::Payload,
                                olp::http::Network::Callback,
                                olp::http::Network::HeaderCallback,
                                olp::http::Network::DataCallback) {
        std::thread([&context]() { context.CancelOperation(); }).detach();

        constexpr auto unused_request_id = 5;
        return olp::http::SendOutcome(unused_request_id);
      });

  ON_CALL(*network_,
          Send(IsGetRequest(OLP_SDK_URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .WillByDefault([](olp::http::NetworkRequest, olp::http::Network::Payload,
                        olp::http::Network::Callback,
                        olp::http::Network::HeaderCallback,
                        olp::http::Network::DataCallback) {
        ADD_FAILURE()
            << "Should not be called. Previous request was cancelled.";
        constexpr auto unused_request_id = 5;
        return olp::http::SendOutcome(unused_request_id);
      });

  auto response =
      olp::dataservice::read::repository::CatalogRepository::GetLatestVersion(
          kHRN, context, request, settings_);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionOnlineOnlyUserCancelled2) {
  olp::client::CancellationContext context;

  auto request = olp::dataservice::read::DataRequest();

  ON_CALL(*network_,
          Send(IsGetRequest(OLP_SDK_URL_LOOKUP_METADATA), _, _, _, _))
      .WillByDefault(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          OLP_SDK_HTTP_RESPONSE_LOOKUP_METADATA));

  ON_CALL(*network_,
          Send(IsGetRequest(OLP_SDK_URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .WillByDefault([&](olp::http::NetworkRequest, olp::http::Network::Payload,
                         olp::http::Network::Callback,
                         olp::http::Network::HeaderCallback,
                         olp::http::Network::DataCallback) {
        std::thread([&]() { context.CancelOperation(); }).detach();

        constexpr auto unused_request_id = 10;
        return olp::http::SendOutcome(unused_request_id);
      });

  auto response =
      olp::dataservice::read::repository::CatalogRepository::GetLatestVersion(
          kHRN, context, request, settings_);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionCancelledBeforeExecution) {
  settings_.retry_settings.timeout = 0;
  olp::client::CancellationContext context;

  auto request = olp::dataservice::read::DataRequest();

  ON_CALL(*network_, Send(_, _, _, _, _))
      .WillByDefault([&](olp::http::NetworkRequest, olp::http::Network::Payload,
                         olp::http::Network::Callback,
                         olp::http::Network::HeaderCallback,
                         olp::http::Network::DataCallback) {
        ADD_FAILURE() << "Should not be called on cancelled operation";
        constexpr auto unused_request_id = 10;
        return olp::http::SendOutcome(unused_request_id);
      });

  context.CancelOperation();
  auto response =
      olp::dataservice::read::repository::CatalogRepository::GetLatestVersion(
          kHRN, context, request, settings_);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionTimeouted) {
  olp::client::CancellationContext context;

  auto request = olp::dataservice::read::DataRequest();

  ON_CALL(*network_,
          Send(IsGetRequest(OLP_SDK_URL_LOOKUP_METADATA), _, _, _, _))
      .WillByDefault(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          OLP_SDK_HTTP_RESPONSE_LOOKUP_METADATA));

  ON_CALL(*network_,
          Send(IsGetRequest(OLP_SDK_URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .WillByDefault([&](olp::http::NetworkRequest, olp::http::Network::Payload,
                         olp::http::Network::Callback,
                         olp::http::Network::HeaderCallback,
                         olp::http::Network::DataCallback) {
        constexpr auto unused_request_id = 10;
        return olp::http::SendOutcome(unused_request_id);
      });

  settings_.retry_settings.timeout = 0;

  auto response =
      olp::dataservice::read::repository::CatalogRepository::GetLatestVersion(
          kHRN, context, request, settings_);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::RequestTimeout,
            response.GetError().GetErrorCode());
}

}  // namespace
