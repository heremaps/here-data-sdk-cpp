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
#include "olp/dataservice/read/CatalogRequest.h"
#include "olp/dataservice/read/CatalogVersionRequest.h"
#include "olp/dataservice/read/DataRequest.h"

#define OLP_SDK_URL_LOOKUP_METADATA \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data::olp-here-test:hereos-internal-test-v2/apis/metadata/v1)"
#define OLP_SDK_HTTP_RESPONSE_LOOKUP_METADATA \
  R"jsonString([{"api":"metadata","version":"v1","baseURL":"https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString"

#define OLP_SDK_URL_LATEST_CATALOG_VERSION \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/versions/latest?startVersion=-1)"

#define OLP_SDK_HTTP_RESPONSE_LATEST_CATALOG_VERSION \
  R"jsonString({"version":4})jsonString"

#define OLP_SDK_URL_CONFIG \
  R"(https://config.data.api.platform.in.here.com/config/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2)"

#define OLP_SDK_HTTP_RESPONSE_CONFIG \
  R"jsonString({"id":"hereos-internal-test","hrn":"hrn:here-dev:data:::hereos-internal-test","name":"hereos-internal-test","summary":"Internal test for hereos","description":"Used for internal testing on the staging olp.","contacts":{},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"tags":[],"billingTags":[],"created":"2018-07-13T20:50:08.425Z","layers":[{"id":"hype-test-prefetch","hrn":"hrn:here-dev:data:::hereos-internal-test:hype-test-prefetch","name":"Hype Test Prefetch","summary":"hype prefetch testing","description":"Layer for hype prefetch testing","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":[],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer_res","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer_res","name":"Resource Test Layer","summary":"testlayer_res","description":"testlayer_res","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer_volatile","ttl":1000,"hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"volatile"},{"id":"testlayer_stream","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"stream"},{"id":"multilevel_testlayer","hrn":"hrn:here-dev:data:::hereos-internal-test:multilevel_testlayer","name":"Multi Level Test Layer","summary":"Multi Level Test Layer","description":"A multi level test layer just for testing","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"hype-test-prefetch-2","hrn":"hrn:here-dev:data:::hereos-internal-test:hype-test-prefetch-2","name":"Hype Test Prefetch2","summary":"Layer for testing hype2 prefetching","description":"Layer for testing hype2 prefetching","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-24T17:52:23.818Z","layerType":"versioned"}],"version":3})jsonString"

#define OLP_SDK_URL_LOOKUP_CONFIG \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/platform/apis/config/v1)"

#define OLP_SDK_HTTP_RESPONSE_LOOKUP_CONFIG \
  R"jsonString([{"api":"config","version":"v1","baseURL":"https://config.data.api.platform.in.here.com/config/v1","parameters":{}},{"api":"pipelines","version":"v1","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}},{"api":"pipelines","version":"v2","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}}])jsonString"

namespace {

using namespace olp::dataservice::read;
using namespace ::testing;
using namespace olp::tests::common;

const std::string kCatalog =
    "hrn:here:data::olp-here-test:hereos-internal-test-v2";
const std::string kLatestVersionCacheKey = kCatalog + "::latestVersion";
const std::string kCatalogCacheKey = kCatalog + "::catalog";
const std::string kMetadataServiceName = "metadata";
const std::string kConfigServiceName = "config";
const std::string kServiceVersion = "v1";
const std::string kMetadataCacheKey =
    kCatalog + "::" + kMetadataServiceName + "::" + kServiceVersion + "::api";
const std::string kConfigCacheKey =
    kCatalog + "::" + kConfigServiceName + "::" + kServiceVersion + "::api";
const std::string kLookupUrl =
    "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/" +
    kCatalog + "/apis/" + kMetadataServiceName + "/" + kServiceVersion;
const auto kHrn = olp::client::HRN::FromString(kCatalog);

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

  auto request = CatalogVersionRequest();

  request.WithFetchOption(CacheOnly);

  model::VersionResponse cached_version;
  cached_version.SetVersion(10);

  EXPECT_CALL(*cache_, Get(kLatestVersionCacheKey, _))
      .Times(1)
      .WillOnce(Return(cached_version));

  auto response = repository::CatalogRepository::GetLatestVersion(
      kHrn, context, request, settings_);

  ASSERT_TRUE(response.IsSuccessful());
  EXPECT_EQ(10, response.GetResult().GetVersion());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionCacheOnlyNotFound) {
  olp::client::CancellationContext context;

  auto request = CatalogVersionRequest();

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
          kHrn, context, request, settings_);

  EXPECT_FALSE(response.IsSuccessful());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionOnlineOnlyNotFound) {
  olp::client::CancellationContext context;

  auto request = CatalogVersionRequest();

  request.WithFetchOption(OnlineOnly);

  ON_CALL(*cache_, Get(_, _))
      .WillByDefault([](const std::string&, const olp::cache::Decoder&) {
        ADD_FAILURE() << "Cache should not be used in OnlineOnly request";
        return boost::any{};
      });

  EXPECT_CALL(*network_,
              Send(IsGetRequest(OLP_SDK_URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1)
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::NOT_FOUND),
                                   ""));

  auto response =
      olp::dataservice::read::repository::CatalogRepository::GetLatestVersion(
          kHrn, context, request, settings_);

  EXPECT_FALSE(response.IsSuccessful());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionOnlineOnlyFound) {
  olp::client::CancellationContext context;

  auto request = CatalogVersionRequest();

  request.WithFetchOption(OnlineOnly);

  ON_CALL(*cache_, Get(_, _))
      .WillByDefault([](const std::string&, const olp::cache::Decoder&) {
        ADD_FAILURE() << "Cache should not be used in OnlineOnly request";
        return boost::any{};
      });

  EXPECT_CALL(*cache_, Put(Eq(kLatestVersionCacheKey), _, _, _)).Times(0);

  EXPECT_CALL(*cache_, Put(Eq(kMetadataCacheKey), _, _, _)).Times(0);

  EXPECT_CALL(*network_,
              Send(IsGetRequest(OLP_SDK_URL_LOOKUP_METADATA), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   OLP_SDK_HTTP_RESPONSE_LOOKUP_METADATA));

  EXPECT_CALL(*network_, Send(IsGetRequest(OLP_SDK_URL_LATEST_CATALOG_VERSION),
                              _, _, _, _))
      .WillOnce(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::OK),
                             OLP_SDK_HTTP_RESPONSE_LATEST_CATALOG_VERSION));

  auto response =
      olp::dataservice::read::repository::CatalogRepository::GetLatestVersion(
          kHrn, context, request, settings_);

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_EQ(4, response.GetResult().GetVersion());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionOnlineOnlyUserCancelled1) {
  olp::client::CancellationContext context;

  auto request = CatalogVersionRequest();

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
          kHrn, context, request, settings_);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionOnlineOnlyUserCancelled2) {
  olp::client::CancellationContext context;

  auto request = CatalogVersionRequest();

  ON_CALL(*network_,
          Send(IsGetRequest(OLP_SDK_URL_LOOKUP_METADATA), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
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
          kHrn, context, request, settings_);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionCancelledBeforeExecution) {
  settings_.retry_settings.timeout = 0;
  olp::client::CancellationContext context;

  auto request = CatalogVersionRequest();

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
          kHrn, context, request, settings_);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionTimeouted) {
  olp::client::CancellationContext context;

  auto request = CatalogVersionRequest();

  ON_CALL(*network_,
          Send(IsGetRequest(OLP_SDK_URL_LOOKUP_METADATA), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
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
          kHrn, context, request, settings_);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::RequestTimeout,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetCatalogOnlineOnlyFound) {
  olp::client::CancellationContext context;

  auto request = CatalogRequest();
  request.WithFetchOption(OnlineOnly);

  ON_CALL(*cache_, Get(_, _))
      .WillByDefault([](const std::string&, const olp::cache::Decoder&) {
        ADD_FAILURE() << "Cache should not be used in OnlineOnly request";
        return boost::any{};
      });

  EXPECT_CALL(*cache_, Put(Eq(kCatalogCacheKey), _, _, _)).Times(0);

  EXPECT_CALL(*cache_, Put(Eq(kConfigCacheKey), _, _, _)).Times(0);

  ON_CALL(*network_, Send(IsGetRequest(OLP_SDK_URL_LOOKUP_CONFIG), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        OLP_SDK_HTTP_RESPONSE_LOOKUP_CONFIG));

  ON_CALL(*network_, Send(IsGetRequest(OLP_SDK_URL_CONFIG), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        OLP_SDK_HTTP_RESPONSE_CONFIG));

  auto response = repository::CatalogRepository::GetCatalog(kHrn, context,
                                                            request, settings_);

  ASSERT_TRUE(response.IsSuccessful());
}

TEST_F(CatalogRepositoryTest, GetCatalogCacheOnlyFound) {
  olp::client::CancellationContext context;

  auto request = CatalogRequest();

  request.WithFetchOption(CacheOnly);

  CatalogResult cached_version;
  cached_version.SetHrn(kCatalog);

  EXPECT_CALL(*cache_, Get(kCatalogCacheKey, _))
      .Times(1)
      .WillOnce(Return(cached_version));

  auto response = repository::CatalogRepository::GetCatalog(kHrn, context,
                                                            request, settings_);

  ASSERT_TRUE(response.IsSuccessful());
  EXPECT_EQ(kCatalog, response.GetResult().GetHrn());
}

TEST_F(CatalogRepositoryTest, GetCatalogCacheOnlyNotFound) {
  olp::client::CancellationContext context;

  auto request = CatalogVersionRequest();

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
          kHrn, context, request, settings_);

  EXPECT_FALSE(response.IsSuccessful());
}

TEST_F(CatalogRepositoryTest, GetCatalogOnlineOnlyNotFound) {
  olp::client::CancellationContext context;

  auto request = CatalogRequest();

  request.WithFetchOption(OnlineOnly);

  ON_CALL(*cache_, Get(_, _))
      .WillByDefault([](const std::string&, const olp::cache::Decoder&) {
        ADD_FAILURE() << "Cache should not be used in OnlineOnly request";
        return boost::any{};
      });

  EXPECT_CALL(*network_,
              Send(IsGetRequest(OLP_SDK_URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1)
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::NOT_FOUND),
                                   ""));

  auto response = repository::CatalogRepository::GetCatalog(kHrn, context,
                                                            request, settings_);

  EXPECT_FALSE(response.IsSuccessful());
}

TEST_F(CatalogRepositoryTest, GetCatalogCancelledBeforeExecution) {
  settings_.retry_settings.timeout = 0;
  olp::client::CancellationContext context;

  auto request = CatalogRequest();

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
  auto response = repository::CatalogRepository::GetCatalog(kHrn, context,
                                                            request, settings_);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetCatalogOnlineOnlyUserCancelled1) {
  olp::client::CancellationContext context;

  auto request = CatalogRequest();

  std::promise<bool> cancelled;

  ON_CALL(*network_, Send(IsGetRequest(OLP_SDK_URL_LOOKUP_CONFIG), _, _, _, _))
      .WillByDefault([&context](olp::http::NetworkRequest,
                                olp::http::Network::Payload,
                                olp::http::Network::Callback,
                                olp::http::Network::HeaderCallback,
                                olp::http::Network::DataCallback) {
        std::thread([&context]() { context.CancelOperation(); }).detach();

        constexpr auto unused_request_id = 5;
        return olp::http::SendOutcome(unused_request_id);
      });

  ON_CALL(*network_, Send(IsGetRequest(OLP_SDK_URL_CONFIG), _, _, _, _))
      .WillByDefault([](olp::http::NetworkRequest, olp::http::Network::Payload,
                        olp::http::Network::Callback,
                        olp::http::Network::HeaderCallback,
                        olp::http::Network::DataCallback) {
        ADD_FAILURE()
            << "Should not be called. Previous request was cancelled.";
        constexpr auto unused_request_id = 5;
        return olp::http::SendOutcome(unused_request_id);
      });

  auto response = repository::CatalogRepository::GetCatalog(kHrn, context,
                                                            request, settings_);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetCatalogOnlineOnlyUserCancelled2) {
  olp::client::CancellationContext context;

  auto request = CatalogRequest();

  ON_CALL(*network_, Send(IsGetRequest(OLP_SDK_URL_LOOKUP_CONFIG), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        OLP_SDK_HTTP_RESPONSE_LOOKUP_CONFIG));

  ON_CALL(*network_, Send(IsGetRequest(OLP_SDK_URL_CONFIG), _, _, _, _))
      .WillByDefault([&](olp::http::NetworkRequest, olp::http::Network::Payload,
                         olp::http::Network::Callback,
                         olp::http::Network::HeaderCallback,
                         olp::http::Network::DataCallback) {
        std::thread([&]() { context.CancelOperation(); }).detach();

        constexpr auto unused_request_id = 10;
        return olp::http::SendOutcome(unused_request_id);
      });

  auto response = repository::CatalogRepository::GetCatalog(kHrn, context,
                                                            request, settings_);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetCatalogTimeout) {
  olp::client::CancellationContext context;

  auto request = CatalogRequest();

  ON_CALL(*network_, Send(IsGetRequest(OLP_SDK_URL_LOOKUP_CONFIG), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        OLP_SDK_HTTP_RESPONSE_LOOKUP_CONFIG));

  ON_CALL(*network_, Send(IsGetRequest(OLP_SDK_URL_CONFIG), _, _, _, _))
      .WillByDefault([&](olp::http::NetworkRequest, olp::http::Network::Payload,
                         olp::http::Network::Callback,
                         olp::http::Network::HeaderCallback,
                         olp::http::Network::DataCallback) {
        constexpr auto unused_request_id = 10;
        return olp::http::SendOutcome(unused_request_id);
      });
  settings_.retry_settings.timeout = 0;

  auto response = repository::CatalogRepository::GetCatalog(kHrn, context,
                                                            request, settings_);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::RequestTimeout,
            response.GetError().GetErrorCode());
}
}  // namespace
