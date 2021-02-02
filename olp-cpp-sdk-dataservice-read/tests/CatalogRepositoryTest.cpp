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
#include "olp/dataservice/read/VersionsRequest.h"

constexpr auto kLookupMetadata =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data::olp-here-test:hereos-internal-test-v2/apis)";
constexpr auto kResponseLookupMetadata =
    R"jsonString([{"api":"metadata","version":"v1","baseURL":"https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";
constexpr auto kLatestCatalogVersion =
    R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/versions/latest?startVersion=-1)";
constexpr auto kResponseLatestCatalogVersion =
    R"jsonString({"version":4})jsonString";
constexpr auto kUrlConfig =
    R"(https://config.data.api.platform.in.here.com/config/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2)";
constexpr auto
    kResponseConfig =
        R"jsonString({"id":"hereos-internal-test","hrn":"hrn:here-dev:data:::hereos-internal-test","name":"hereos-internal-test","summary":"Internal test for hereos","description":"Used for internal testing on the staging olp.","contacts":{},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"tags":[],"billingTags":[],"created":"2018-07-13T20:50:08.425Z","layers":[{"id":"hype-test-prefetch","hrn":"hrn:here-dev:data:::hereos-internal-test:hype-test-prefetch","name":"Hype Test Prefetch","summary":"hype prefetch testing","description":"Layer for hype prefetch testing","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":[],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer_res","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer_res","name":"Resource Test Layer","summary":"testlayer_res","description":"testlayer_res","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer_volatile","ttl":1000,"hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"volatile"},{"id":"testlayer_stream","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"stream"},{"id":"multilevel_testlayer","hrn":"hrn:here-dev:data:::hereos-internal-test:multilevel_testlayer","name":"Multi Level Test Layer","summary":"Multi Level Test Layer","description":"A multi level test layer just for testing","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"hype-test-prefetch-2","hrn":"hrn:here-dev:data:::hereos-internal-test:hype-test-prefetch-2","name":"Hype Test Prefetch2","summary":"Layer for testing hype2 prefetching","description":"Layer for testing hype2 prefetching","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-24T17:52:23.818Z","layerType":"versioned"}],"version":3})jsonString";
constexpr auto kUrlLookupConfig =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/platform/apis)";
constexpr auto kResponseLookupConfig =
    R"jsonString([{"api":"config","version":"v1","baseURL":"https://config.data.api.platform.in.here.com/config/v1","parameters":{}},{"api":"pipelines","version":"v1","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}},{"api":"pipelines","version":"v2","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}}])jsonString";
constexpr auto kStartVersion = 3;
constexpr auto kEndVersion = 4;
constexpr auto kUrlVersionsList =
    R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/versions?endVersion=4&startVersion=3)";
constexpr auto kUrlVersionsListStartMinus =
    R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/versions?endVersion=4&startVersion=-1)";
constexpr auto kHttpResponse =
    R"jsonString({"versions":[{"version":4,"timestamp":1547159598712,"partitionCounts":{"testlayer":5,"testlayer_res":1,"multilevel_testlayer":33, "hype-test-prefetch-2":7,"testlayer_gzip":1,"hype-test-prefetch":7},"dependencies":[ { "hrn":"hrn:here:data::olp-here-test:hereos-internal-test-v2","version":0,"direct":false},{"hrn":"hrn:here:data:::hereos-internal-test-v2","version":0,"direct":false }]}]})jsonString";

namespace {

namespace read = olp::dataservice::read;
namespace model = olp::dataservice::read::model;
namespace repository = olp::dataservice::read::repository;
using olp::client::ApiLookupClient;
using ::testing::_;

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
const std::string kVersionInfosCacheKey = kCatalog + "::3::4::versionInfos";

const auto kHrn = olp::client::HRN::FromString(kCatalog);

class CatalogRepositoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    cache_ = std::make_shared<testing::NiceMock<CacheMock>>();
    network_ = std::make_shared<testing::NiceMock<NetworkMock>>();

    settings_.network_request_handler = network_;
    settings_.cache = cache_;

    lookup_client_ =
        std::make_shared<olp::client::ApiLookupClient>(kHrn, settings_);
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
  std::shared_ptr<olp::client::ApiLookupClient> lookup_client_;
};

TEST_F(CatalogRepositoryTest, GetLatestVersionCacheOnlyFound) {
  olp::client::CancellationContext context;

  auto request = read::CatalogVersionRequest();

  request.WithFetchOption(read::CacheOnly);

  model::VersionResponse cached_version;
  cached_version.SetVersion(10);

  EXPECT_CALL(*cache_, Get(kLatestVersionCacheKey, _))
      .Times(1)
      .WillOnce(testing::Return(cached_version));

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetLatestVersion(request, context);

  ASSERT_TRUE(response.IsSuccessful());
  EXPECT_EQ(10, response.GetResult().GetVersion());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionCacheOnlyNotFound) {
  olp::client::CancellationContext context;

  auto request = read::CatalogVersionRequest();

  request.WithFetchOption(read::CacheOnly);

  EXPECT_CALL(*cache_, Get(_, _))
      .Times(1)
      .WillOnce(testing::Return(boost::any{}));

  ON_CALL(*network_, Send(_, _, _, _, _))
      .WillByDefault([](olp::http::NetworkRequest, olp::http::Network::Payload,
                        olp::http::Network::Callback,
                        olp::http::Network::HeaderCallback,
                        olp::http::Network::DataCallback) {
        ADD_FAILURE() << "Should not be called with CacheOnly";
        return olp::http::SendOutcome(olp::http::ErrorCode::UNKNOWN_ERROR);
      });

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetLatestVersion(request, context);

  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::NotFound);
}

TEST_F(CatalogRepositoryTest, GetLatestVersionOnlineOnlyNotFound) {
  olp::client::CancellationContext context;

  auto request = read::CatalogVersionRequest();

  request.WithFetchOption(read::OnlineOnly);

  ON_CALL(*cache_, Get(_, _))
      .WillByDefault([](const std::string&, const olp::cache::Decoder&) {
        ADD_FAILURE() << "Cache should not be used in OnlineOnly request";
        return boost::any{};
      });

  EXPECT_CALL(*cache_, Get(testing::Eq(kMetadataCacheKey), _))
      .WillOnce(testing::Return(boost::any()));

  EXPECT_CALL(*network_, Send(IsGetRequest(kLookupMetadata), _, _, _, _))
      .Times(1)
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::NOT_FOUND),
                                   ""));

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetLatestVersion(request, context);

  EXPECT_FALSE(response.IsSuccessful());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionOnlineOnlyFound) {
  olp::client::CancellationContext context;

  auto request = read::CatalogVersionRequest();

  request.WithFetchOption(read::OnlineOnly);

  ON_CALL(*cache_, Get(_, _))
      .WillByDefault([](const std::string&, const olp::cache::Decoder&) {
        ADD_FAILURE() << "Cache should not be used in OnlineOnly request";
        return boost::any{};
      });

  EXPECT_CALL(*cache_, Get(testing::Eq(kMetadataCacheKey), _))
      .WillOnce(testing::Return(boost::any()));

  EXPECT_CALL(*network_, Send(IsGetRequest(kLookupMetadata), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kResponseLookupMetadata));

  EXPECT_CALL(*cache_, Put(testing::Eq(kMetadataCacheKey), _, _, _)).Times(1);

  EXPECT_CALL(*network_, Send(IsGetRequest(kLatestCatalogVersion), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kResponseLatestCatalogVersion));

  EXPECT_CALL(*cache_, Put(testing::Eq(kLatestVersionCacheKey), _, _, _))
      .Times(0);

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetLatestVersion(request, context);

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_EQ(4, response.GetResult().GetVersion());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionOnlineOnlyUserCancelled1) {
  olp::client::CancellationContext context;

  auto request = read::CatalogVersionRequest();

  std::promise<bool> cancelled;

  ON_CALL(*network_, Send(IsGetRequest(kLookupMetadata), _, _, _, _))
      .WillByDefault([&context](olp::http::NetworkRequest,
                                olp::http::Network::Payload,
                                olp::http::Network::Callback,
                                olp::http::Network::HeaderCallback,
                                olp::http::Network::DataCallback) {
        std::thread([&context]() { context.CancelOperation(); }).detach();

        constexpr auto unused_request_id = 5;
        return olp::http::SendOutcome(unused_request_id);
      });

  ON_CALL(*network_, Send(IsGetRequest(kLatestCatalogVersion), _, _, _, _))
      .WillByDefault([](olp::http::NetworkRequest, olp::http::Network::Payload,
                        olp::http::Network::Callback,
                        olp::http::Network::HeaderCallback,
                        olp::http::Network::DataCallback) {
        ADD_FAILURE()
            << "Should not be called. Previous request was cancelled.";
        constexpr auto unused_request_id = 5;
        return olp::http::SendOutcome(unused_request_id);
      });

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetLatestVersion(request, context);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionOnlineOnlyUserCancelled2) {
  olp::client::CancellationContext context;

  auto request = read::CatalogVersionRequest();

  ON_CALL(*network_, Send(IsGetRequest(kLookupMetadata), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        kResponseLookupMetadata));

  ON_CALL(*network_, Send(IsGetRequest(kLatestCatalogVersion), _, _, _, _))
      .WillByDefault([&](olp::http::NetworkRequest, olp::http::Network::Payload,
                         olp::http::Network::Callback,
                         olp::http::Network::HeaderCallback,
                         olp::http::Network::DataCallback) {
        std::thread([&]() { context.CancelOperation(); }).detach();

        constexpr auto unused_request_id = 10;
        return olp::http::SendOutcome(unused_request_id);
      });

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetLatestVersion(request, context);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionCancelledBeforeExecution) {
  settings_.retry_settings.timeout = 0;
  olp::client::CancellationContext context;

  auto request = read::CatalogVersionRequest();

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

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetLatestVersion(request, context);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetLatestVersionTimeouted) {
  olp::client::CancellationContext context;

  auto request = read::CatalogVersionRequest();

  ON_CALL(*network_, Send(IsGetRequest(kLookupMetadata), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        kResponseLookupMetadata));

  ON_CALL(*network_, Send(IsGetRequest(kLatestCatalogVersion), _, _, _, _))
      .WillByDefault([&](olp::http::NetworkRequest, olp::http::Network::Payload,
                         olp::http::Network::Callback,
                         olp::http::Network::HeaderCallback,
                         olp::http::Network::DataCallback) {
        constexpr auto unused_request_id = 10;
        return olp::http::SendOutcome(unused_request_id);
      });

  settings_.retry_settings.timeout = 0;

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetLatestVersion(request, context);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::RequestTimeout,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetCatalogOnlineOnlyFound) {
  olp::client::CancellationContext context;

  auto request = read::CatalogRequest();
  request.WithFetchOption(read::OnlineOnly);

  ON_CALL(*cache_, Get(_, _))
      .WillByDefault([](const std::string&, const olp::cache::Decoder&) {
        ADD_FAILURE() << "Cache should not be used in OnlineOnly request";
        return boost::any{};
      });

  EXPECT_CALL(*cache_, Put(testing::Eq(kCatalogCacheKey), _, _, _)).Times(0);

  EXPECT_CALL(*cache_, Put(testing::Eq(kConfigCacheKey), _, _, _)).Times(0);

  ON_CALL(*network_, Send(IsGetRequest(kUrlLookupConfig), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        kResponseLookupConfig));

  ON_CALL(*network_, Send(IsGetRequest(kUrlConfig), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        kResponseConfig));

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetCatalog(request, context);

  ASSERT_TRUE(response.IsSuccessful());
}

TEST_F(CatalogRepositoryTest, GetCatalogCacheOnlyFound) {
  olp::client::CancellationContext context;

  auto request = read::CatalogRequest();

  request.WithFetchOption(read::CacheOnly);

  read::CatalogResult cached_version;
  cached_version.SetHrn(kCatalog);

  EXPECT_CALL(*cache_, Get(kCatalogCacheKey, _))
      .Times(1)
      .WillOnce(testing::Return(cached_version));

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetCatalog(request, context);

  ASSERT_TRUE(response.IsSuccessful());
  EXPECT_EQ(kCatalog, response.GetResult().GetHrn());
}

TEST_F(CatalogRepositoryTest, GetCatalogCacheOnlyNotFound) {
  olp::client::CancellationContext context;

  auto request = read::CatalogVersionRequest();

  request.WithFetchOption(read::CacheOnly);

  EXPECT_CALL(*cache_, Get(_, _))
      .Times(1)
      .WillOnce(testing::Return(boost::any{}));

  ON_CALL(*network_, Send(_, _, _, _, _))
      .WillByDefault([](olp::http::NetworkRequest, olp::http::Network::Payload,
                        olp::http::Network::Callback,
                        olp::http::Network::HeaderCallback,
                        olp::http::Network::DataCallback) {
        ADD_FAILURE() << "Should not be called with CacheOnly";
        return olp::http::SendOutcome(olp::http::ErrorCode::UNKNOWN_ERROR);
      });

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetLatestVersion(request, context);

  EXPECT_FALSE(response.IsSuccessful());
}

TEST_F(CatalogRepositoryTest, GetCatalogOnlineOnlyNotFound) {
  olp::client::CancellationContext context;

  auto request = read::CatalogRequest();

  request.WithFetchOption(read::OnlineOnly);

  ON_CALL(*cache_, Get(_, _))
      .WillByDefault([](const std::string&, const olp::cache::Decoder&) {
        ADD_FAILURE() << "Cache should not be used in OnlineOnly request";
        return boost::any{};
      });

  EXPECT_CALL(*network_, Send(IsGetRequest(kUrlLookupConfig), _, _, _, _))
      .Times(1)
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::NOT_FOUND),
                                   ""));

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetCatalog(request, context);

  EXPECT_FALSE(response.IsSuccessful());
}

TEST_F(CatalogRepositoryTest, GetCatalogCancelledBeforeExecution) {
  settings_.retry_settings.timeout = 0;
  olp::client::CancellationContext context;

  auto request = read::CatalogRequest();

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

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetCatalog(request, context);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetCatalogOnlineOnlyUserCancelled1) {
  olp::client::CancellationContext context;

  auto request = read::CatalogRequest();

  std::promise<bool> cancelled;

  ON_CALL(*network_, Send(IsGetRequest(kUrlLookupConfig), _, _, _, _))
      .WillByDefault([&context](olp::http::NetworkRequest,
                                olp::http::Network::Payload,
                                olp::http::Network::Callback,
                                olp::http::Network::HeaderCallback,
                                olp::http::Network::DataCallback) {
        std::thread([&context]() { context.CancelOperation(); }).detach();

        constexpr auto unused_request_id = 5;
        return olp::http::SendOutcome(unused_request_id);
      });

  ON_CALL(*network_, Send(IsGetRequest(kUrlConfig), _, _, _, _))
      .WillByDefault([](olp::http::NetworkRequest, olp::http::Network::Payload,
                        olp::http::Network::Callback,
                        olp::http::Network::HeaderCallback,
                        olp::http::Network::DataCallback) {
        ADD_FAILURE()
            << "Should not be called. Previous request was cancelled.";
        constexpr auto unused_request_id = 5;
        return olp::http::SendOutcome(unused_request_id);
      });

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetCatalog(request, context);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetCatalogOnlineOnlyUserCancelled2) {
  olp::client::CancellationContext context;

  auto request = read::CatalogRequest();

  ON_CALL(*network_, Send(IsGetRequest(kUrlLookupConfig), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        kResponseLookupConfig));

  ON_CALL(*network_, Send(IsGetRequest(kUrlConfig), _, _, _, _))
      .WillByDefault([&](olp::http::NetworkRequest, olp::http::Network::Payload,
                         olp::http::Network::Callback,
                         olp::http::Network::HeaderCallback,
                         olp::http::Network::DataCallback) {
        std::thread([&]() { context.CancelOperation(); }).detach();

        constexpr auto unused_request_id = 10;
        return olp::http::SendOutcome(unused_request_id);
      });

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetCatalog(request, context);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetCatalogTimeout) {
  olp::client::CancellationContext context;

  auto request = read::CatalogRequest();

  ON_CALL(*network_, Send(IsGetRequest(kUrlLookupConfig), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        kResponseLookupConfig));

  ON_CALL(*network_, Send(IsGetRequest(kUrlConfig), _, _, _, _))
      .WillByDefault([&](olp::http::NetworkRequest, olp::http::Network::Payload,
                         olp::http::Network::Callback,
                         olp::http::Network::HeaderCallback,
                         olp::http::Network::DataCallback) {
        constexpr auto unused_request_id = 10;
        return olp::http::SendOutcome(unused_request_id);
      });
  settings_.retry_settings.timeout = 0;

  ApiLookupClient lookup_client(kHrn, settings_);
  repository::CatalogRepository repository(kHrn, settings_, lookup_client);
  auto response = repository.GetCatalog(request, context);

  ASSERT_FALSE(response.IsSuccessful());
  EXPECT_EQ(olp::client::ErrorCode::RequestTimeout,
            response.GetError().GetErrorCode());
}

TEST_F(CatalogRepositoryTest, GetVersionsList) {
  {
    SCOPED_TRACE("Get versions list");

    olp::client::CancellationContext context;
    auto request = read::VersionsRequest()
                       .WithStartVersion(kStartVersion)
                       .WithEndVersion(kEndVersion);

    ON_CALL(*network_, Send(IsGetRequest(kLookupMetadata), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kResponseLookupMetadata));

    ON_CALL(*network_, Send(IsGetRequest(kUrlVersionsList), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kHttpResponse));

    ApiLookupClient lookup_client(kHrn, settings_);
    repository::CatalogRepository repository(kHrn, settings_, lookup_client);
    auto response = repository.GetVersionsList(request, context);

    ASSERT_TRUE(response.IsSuccessful());
    auto result = response.GetResult();

    ASSERT_EQ(1u, result.GetVersions().size());
    ASSERT_EQ(4, result.GetVersions().front().GetVersion());
    ASSERT_EQ(2u, result.GetVersions().front().GetDependencies().size());
    ASSERT_EQ(6u, result.GetVersions().front().GetPartitionCounts().size());
  }
  {
    SCOPED_TRACE("Get versions list start version -1");

    olp::client::CancellationContext context;
    auto request = read::VersionsRequest().WithStartVersion(-1).WithEndVersion(
        kEndVersion);

    ON_CALL(*network_, Send(IsGetRequest(kLookupMetadata), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kResponseLookupMetadata));

    ON_CALL(*network_,
            Send(IsGetRequest(kUrlVersionsListStartMinus), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kHttpResponse));

    ApiLookupClient lookup_client(kHrn, settings_);
    repository::CatalogRepository repository(kHrn, settings_, lookup_client);
    auto response = repository.GetVersionsList(request, context);

    ASSERT_TRUE(response.IsSuccessful());
    auto result = response.GetResult();

    ASSERT_EQ(1u, result.GetVersions().size());
    ASSERT_EQ(4, result.GetVersions().front().GetVersion());
    ASSERT_EQ(2u, result.GetVersions().front().GetDependencies().size());
    ASSERT_EQ(6u, result.GetVersions().front().GetPartitionCounts().size());
  }
  {
    SCOPED_TRACE("Get versions list response forbiden");

    olp::client::CancellationContext context;
    auto request = read::VersionsRequest()
                       .WithStartVersion(kStartVersion)
                       .WithEndVersion(kEndVersion);

    ON_CALL(*network_, Send(IsGetRequest(kLookupMetadata), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kResponseLookupMetadata));

    ON_CALL(*network_, Send(IsGetRequest(kUrlVersionsList), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::FORBIDDEN),
                               "Forbidden"));

    ApiLookupClient lookup_client(kHrn, settings_);
    repository::CatalogRepository repository(kHrn, settings_, lookup_client);
    auto response = repository.GetVersionsList(request, context);

    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::client::ErrorCode::AccessDenied,
              response.GetError().GetErrorCode());
  }
}
}  // namespace
