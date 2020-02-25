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

#include "repositories/PartitionsRepository.h"

#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/model/Partitions.h>

// clang-format off
#include "generated/parser/PartitionsParser.h"
#include <olp/core/generated/parser/JsonParser.h>
// clang-format on

namespace {
using namespace olp;
using namespace client;
using namespace dataservice::read;
using namespace olp::tests::common;

const std::string kCatalog =
    "hrn:here:data::olp-here-test:hereos-internal-test-v2";
const std::string kLayerId = "test_layer";
const std::string kPartitionId = "1111";
constexpr int kVersion = 4;

const std::string kOlpSdkUrlLookupMetadata =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/)" +
    kCatalog + R"(/apis/query/v1)";
const std::string kOlpSdkHttpResponseLookupMetadata =
    R"jsonString([{"api":"metadata","version":"v1","baseURL":"https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";
const std::string kCacheKeyMetadata = kCatalog + "::query::v1::api";

const std::string kOlpSdkUrlPartitionByIdNoVersion =
    R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layers/)" +
    kLayerId + R"(/partitions?partition=)" + kPartitionId;
const std::string kOlpSdkUrlPartitionById = kOlpSdkUrlPartitionByIdNoVersion +
                                            R"(&version=)" +
                                            std::to_string(kVersion);
const std::string kOlpSdkHttpResponsePartitionById =
    R"jsonString({ "partitions": [{"version":42,"partition":")jsonString" +
    kPartitionId +
    R"jsonString(","layer":"olp-cpp-sdk-ingestion-test-volatile-layer","dataHandle":"PartitionsRepositoryTest-partitionId"}]})jsonString";

const std::string kOlpSdkUrlLookupConfig =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/platform/apis/config/v1)";

const std::string kOlpSdkHttpResponseLookupConfig =
    R"jsonString([{"api":"config","version":"v1","baseURL":"https://config.data.api.platform.in.here.com/config/v1","parameters":{}},{"api":"pipelines","version":"v1","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}},{"api":"pipelines","version":"v2","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}}])jsonString";

const std::string kOlpSdkUrlConfig =
    R"(https://config.data.api.platform.in.here.com/config/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2)";

const std::string
    kOlpSdkHttpResponseConfig =
        R"jsonString({"id":"hereos-internal-test","hrn":"hrn:here-dev:data:::hereos-internal-test","name":"hereos-internal-test","summary":"Internal test for hereos","description":"Used for internal testing on the staging olp.","contacts":{},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"tags":[],"billingTags":[],"created":"2018-07-13T20:50:08.425Z","layers":[{"id":"hype-test-prefetch","hrn":"hrn:here-dev:data:::hereos-internal-test:hype-test-prefetch","name":"Hype Test Prefetch","summary":"hype prefetch testing","description":"Layer for hype prefetch testing","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":[],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer_res","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer_res","name":"Resource Test Layer","summary":"testlayer_res","description":"testlayer_res","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer_volatile","ttl":1000,"hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"volatile"},{"id":"testlayer_stream","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"stream"},{"id":"multilevel_testlayer","hrn":"hrn:here-dev:data:::hereos-internal-test:multilevel_testlayer","name":"Multi Level Test Layer","summary":"Multi Level Test Layer","description":"A multi level test layer just for testing","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"hype-test-prefetch-2","hrn":"hrn:here-dev:data:::hereos-internal-test:hype-test-prefetch-2","name":"Hype Test Prefetch2","summary":"Layer for testing hype2 prefetching","description":"Layer for testing hype2 prefetching","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-24T17:52:23.818Z","layerType":"versioned"}],"version":3})jsonString";

const std::string kOlpSdkUrlLookupMetadata2 =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data::olp-here-test:hereos-internal-test-v2/apis/metadata/v1)";
const std::string kOlpSdkHttpResponseLookupMetadata2 =
    R"jsonString([{"api":"metadata","version":"v1","baseURL":"https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";

const std::string kOlpSdkUrlPartitions =
    R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layers/testlayer_volatile/partitions)";

const std::string kOlpSdkHttpResponsePartitions =
    R"jsonString({ "partitions": [{"version":4,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"},{"version":4,"partition":"270","layer":"testlayer","dataHandle":"30640762-b429-47b9-9ed6-7a4af6086e8e"},{"version":4,"partition":"3","layer":"testlayer","dataHandle":"data:SomethingBaH!"},{"version":4,"partition":"here_van_wc2018_pool","layer":"testlayer","dataHandle":"bcde4cc0-2678-40e9-b791-c630faee14c3"}]})jsonString";

TEST(PartitionsRepositoryTest, GetPartitionById) {
  using namespace testing;
  using testing::Return;

  auto cache = std::make_shared<testing::StrictMock<CacheMock>>();
  auto network = std::make_shared<testing::StrictMock<NetworkMock>>();

  const auto catalog_hrn = HRN::FromString(kCatalog);

  OlpClientSettings settings;
  settings.cache = cache;
  settings.network_request_handler = network;
  settings.retry_settings.timeout = 1;

  const DataRequest request{
      DataRequest().WithPartitionId(kPartitionId).WithVersion(kVersion)};
  const std::string part_cache_key =
      kCatalog + "::" + kLayerId + "::" + kPartitionId + "::";

  const std::string cache_key_no_version = part_cache_key + "partition";
  const std::string cache_key =
      part_cache_key + std::to_string(kVersion) + "::partition";

  auto setup_online_only_mocks = [&]() {
    ON_CALL(*cache, Get(_, _))
        .WillByDefault([](const std::string&, const olp::cache::Decoder&) {
          ADD_FAILURE() << "Cache should not be used in OnlineOnly request";
          return boost::any{};
        });
  };

  auto setup_positive_metadata_mocks = [&]() {
    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlLookupMetadata), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kOlpSdkHttpResponseLookupMetadata));

    EXPECT_CALL(*cache, Put(Eq(kCacheKeyMetadata), _, _, _)).Times(1);
  };

  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] positive");

    const std::string query_cache_response =
        R"jsonString({"version":4,"partition":"1111","layer":"testlayer","dataHandle":"qwerty"})jsonString";

    EXPECT_CALL(*cache, Get(cache_key, _))
        .Times(1)
        .WillOnce(
            Return(parser::parse<model::Partition>(query_cache_response)));

    client::CancellationContext context;
    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithFetchOption(CacheOnly), settings);

    ASSERT_TRUE(response.IsSuccessful());
    const auto& result = response.GetResult();
    const auto& partitions = result.GetPartitions();
    EXPECT_EQ(partitions.size(), 1);
    const auto& partition = partitions.front();
    EXPECT_EQ(partition.GetDataHandle(), "qwerty");
    EXPECT_EQ(partition.GetVersion().value_or(0), kVersion);
    EXPECT_EQ(partition.GetPartition(), kPartitionId);

    Mock::VerifyAndClearExpectations(cache.get());
  }
  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] negative");

    EXPECT_CALL(*cache, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(boost::any()));

    client::CancellationContext context;
    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithFetchOption(CacheOnly), settings);

    ASSERT_FALSE(response.IsSuccessful());
    const auto& result = response.GetError();
    EXPECT_EQ(result.GetErrorCode(), ErrorCode::NotFound);

    Mock::VerifyAndClearExpectations(cache.get());
  }
  {
    SCOPED_TRACE("Fetch with missing partition id");

    client::CancellationContext context;
    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithPartitionId(boost::none), settings);

    ASSERT_FALSE(response.IsSuccessful());
    const auto& result = response.GetError();
    EXPECT_EQ(result.GetErrorCode(), ErrorCode::PreconditionFailed);

    Mock::VerifyAndClearExpectations(cache.get());
  }

  {
    SCOPED_TRACE("Fetch from network");
    setup_online_only_mocks();
    setup_positive_metadata_mocks();

    client::CancellationContext context;
    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlPartitionById), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kOlpSdkHttpResponsePartitionById));

    EXPECT_CALL(*cache, Put(Eq(cache_key), _, _, _)).Times(1);

    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithFetchOption(OnlineOnly), settings);

    ASSERT_TRUE(response.IsSuccessful());
    const auto& partitions = response.GetResult().GetPartitions();
    EXPECT_EQ(partitions.size(), 1);
    const auto& partition = partitions.front();
    EXPECT_EQ(partition.GetDataHandle(),
              "PartitionsRepositoryTest-partitionId");
    EXPECT_EQ(partition.GetVersion().value_or(0), 42);
    EXPECT_EQ(partition.GetPartition(), "1111");

    Mock::VerifyAndClearExpectations(cache.get());
  }
  {
    SCOPED_TRACE("Fetch from network with missing version");
    setup_online_only_mocks();
    setup_positive_metadata_mocks();

    client::CancellationContext context;
    EXPECT_CALL(*network, Send(IsGetRequest(kOlpSdkUrlPartitionByIdNoVersion),
                               _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kOlpSdkHttpResponsePartitionById));
    EXPECT_CALL(*cache, Put(Eq(cache_key_no_version), _, _, _)).Times(1);

    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request)
            .WithFetchOption(OnlineOnly)
            .WithVersion(boost::none),
        settings);

    ASSERT_TRUE(response.IsSuccessful());
    const auto& partitions = response.GetResult().GetPartitions();
    EXPECT_EQ(partitions.size(), 1);
    const auto& partition = partitions.front();
    EXPECT_EQ(partition.GetDataHandle(),
              "PartitionsRepositoryTest-partitionId");
    EXPECT_EQ(partition.GetVersion().value_or(0), 42);
    EXPECT_EQ(partition.GetPartition(), "1111");

    Mock::VerifyAndClearExpectations(cache.get());
  }

  {
    SCOPED_TRACE("Network error at lookup state propagated to the user");
    setup_online_only_mocks();

    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlLookupMetadata), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::UNAUTHORIZED),
                               "Inappropriate"));

    client::CancellationContext context;
    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithFetchOption(OnlineOnly), settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::AccessDenied);
    Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Network error at partition state propagated to the user");
    setup_online_only_mocks();
    setup_positive_metadata_mocks();

    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlPartitionById), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::UNAUTHORIZED),
                               "{Inappropriate}"));

    client::CancellationContext context;
    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithFetchOption(OnlineOnly), settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::AccessDenied);
    Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE(
        "Network error 403 clears cache and is propagated to the user");
    setup_online_only_mocks();
    setup_positive_metadata_mocks();
    EXPECT_CALL(*cache, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(boost::any()));

    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlPartitionById), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::FORBIDDEN),
                                     "{Inappropriate}"));

    client::CancellationContext context;
    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithFetchOption(OnlineOnly), settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::AccessDenied);
    Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE(
        "Network request cancelled by network internally at lookup state");
    setup_online_only_mocks();

    client::CancellationContext context;
    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlLookupMetadata), _, _, _, _))
        .Times(1)
        .WillOnce([=](olp::http::NetworkRequest request,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback header_callback,
                      olp::http::Network::DataCallback data_callback)
                      -> olp::http::SendOutcome {
          return olp::http::SendOutcome(olp::http::ErrorCode::CANCELLED_ERROR);
        });

    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithFetchOption(OnlineOnly), settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE(
        "Network request cancelled by network internally at partition state");
    setup_online_only_mocks();
    setup_positive_metadata_mocks();

    client::CancellationContext context;
    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlPartitionById), _, _, _, _))
        .Times(1)
        .WillOnce([=](olp::http::NetworkRequest request,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback header_callback,
                      olp::http::Network::DataCallback data_callback)
                      -> olp::http::SendOutcome {
          return olp::http::SendOutcome(olp::http::ErrorCode::CANCELLED_ERROR);
        });

    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithFetchOption(OnlineOnly), settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE("Network request timed out at lookup state");
    setup_online_only_mocks();

    client::CancellationContext context;
    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlLookupMetadata), _, _, _, _))
        .Times(1)
        .WillOnce([=](olp::http::NetworkRequest request,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback header_callback,
                      olp::http::Network::DataCallback data_callback)
                      -> olp::http::SendOutcome {
          // note no network response thread spawns
          constexpr auto unused_request_id = 12;
          return olp::http::SendOutcome(unused_request_id);
        });
    EXPECT_CALL(*network, Cancel(_)).Times(1).WillOnce(Return());

    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithFetchOption(OnlineOnly), settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::RequestTimeout);
    Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE("Network request timed out at partition state");
    setup_online_only_mocks();
    setup_positive_metadata_mocks();

    client::CancellationContext context;
    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlPartitionById), _, _, _, _))
        .Times(1)
        .WillOnce([=](olp::http::NetworkRequest request,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback header_callback,
                      olp::http::Network::DataCallback data_callback)
                      -> olp::http::SendOutcome {
          // note no network response thread spawns
          constexpr auto unused_request_id = 12;
          return olp::http::SendOutcome(unused_request_id);
        });
    EXPECT_CALL(*network, Cancel(_)).Times(1).WillOnce(Return());

    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithFetchOption(OnlineOnly), settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::RequestTimeout);
    Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE("Network request cancelled by user at lookup state");
    setup_online_only_mocks();

    client::CancellationContext context;
    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlLookupMetadata), _, _, _, _))
        .Times(1)
        .WillOnce(
            [=, &context](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback)
                -> olp::http::SendOutcome {
              // spawn a 'user' response of cancelling
              std::thread([&context]() { context.CancelOperation(); }).detach();

              // note no network response thread spawns

              constexpr auto unused_request_id = 12;
              return olp::http::SendOutcome(unused_request_id);
            });
    EXPECT_CALL(*network, Cancel(_)).Times(1).WillOnce(Return());

    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithFetchOption(OnlineOnly), settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Network request cancelled by user at patition state");
    setup_online_only_mocks();
    setup_positive_metadata_mocks();

    client::CancellationContext context;
    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlPartitionById), _, _, _, _))
        .Times(1)
        .WillOnce(
            [=, &context](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback)
                -> olp::http::SendOutcome {
              // spawn a 'user' response of cancelling
              std::thread([&context]() { context.CancelOperation(); }).detach();

              // note no network response thread spawns

              constexpr auto unused_request_id = 12;
              return olp::http::SendOutcome(unused_request_id);
            });
    EXPECT_CALL(*network, Cancel(_)).Times(1).WillOnce(Return());

    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithFetchOption(OnlineOnly), settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE("Network request cancelled before execution setup");
    setup_online_only_mocks();

    client::CancellationContext context;
    context.CancelOperation();
    auto response = repository::PartitionsRepository::GetPartitionById(
        catalog_hrn, kLayerId, context,
        DataRequest(request).WithFetchOption(OnlineOnly), settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network.get());
  }
}

TEST(PartitionsRepositoryTest, GetVolatilePartitions) {
  using namespace testing;
  using testing::Return;

  std::shared_ptr<cache::KeyValueCache> default_cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache({});

  auto mock_network = std::make_shared<NetworkMock>();
  const auto catalog = HRN::FromString(kCatalog);

  EXPECT_CALL(*mock_network,
              Send(IsGetRequest(kOlpSdkUrlLookupConfig), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kOlpSdkHttpResponseLookupConfig));

  EXPECT_CALL(*mock_network, Send(IsGetRequest(kOlpSdkUrlConfig), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kOlpSdkHttpResponseConfig));

  EXPECT_CALL(*mock_network,
              Send(IsGetRequest(kOlpSdkUrlLookupMetadata2), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kOlpSdkHttpResponseLookupMetadata2));

  EXPECT_CALL(*mock_network,
              Send(IsGetRequest(kOlpSdkUrlPartitions), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kOlpSdkHttpResponsePartitions));

  const auto layer = "testlayer_volatile";

  {
    SCOPED_TRACE("Successfull fetch from network");

    OlpClientSettings settings;
    settings.cache = default_cache;
    settings.network_request_handler = mock_network;
    settings.retry_settings.timeout = 1;

    CancellationContext context;
    PartitionsRequest request;
    auto response = repository::PartitionsRepository::GetVolatilePartitions(
        catalog, layer, context, request, settings);

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    EXPECT_EQ(response.GetResult().GetPartitions().size(), 4);
  }

  {
    SCOPED_TRACE("Successfull fetch from only cache");

    OlpClientSettings settings;
    settings.cache = default_cache;
    settings.retry_settings.timeout = 0;

    CancellationContext context;
    PartitionsRequest request;
    // Volatile layer must survive attempt to fetch versioned data
    request.WithFetchOption(FetchOptions::CacheOnly).WithVersion(10);

    auto cache_only_response =
        repository::PartitionsRepository::GetVolatilePartitions(
            catalog, layer, context, request, settings);

    ASSERT_TRUE(cache_only_response.IsSuccessful())
        << cache_only_response.GetError().GetMessage();
    EXPECT_EQ(cache_only_response.GetResult().GetPartitions().size(), 4);
  }
}
}  // namespace
