/*
 * Copyright (C) 2019-2022 HERE Europe B.V.
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
#include <olp/core/utils/Url.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/TileRequest.h>
#include <olp/dataservice/read/model/Partitions.h>

// clang-format off
#include "generated/parser/PartitionsParser.h"
#include <olp/core/generated/parser/JsonParser.h>
// clang-format on

namespace {
using olp::client::ApiLookupClient;
using olp::client::ErrorCode;
using olp::client::HRN;
using olp::client::OlpClientSettings;
using olp::dataservice::read::DataRequest;
using olp::geo::TileKey;
using testing::_;

namespace parser = olp::parser;
namespace read = olp::dataservice::read;
namespace model = read::model;
namespace client = olp::client;
namespace repository = read::repository;
namespace cache = olp::cache;

const std::string kCatalog =
    "hrn:here:data::olp-here-test:hereos-internal-test-v2";
const std::string kVersionedLayerId = "testlayer";
const std::string kVolatileLayerId = "testlayer_volatile";
const std::string kPartitionId = "1111";
const std::string kInvalidPartitionId = "2222";
constexpr int kVersion = 100;

const std::string kOlpSdkUrlLookupQuery =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/)" +
    kCatalog + R"(/apis)";
const std::string kOlpSdkHttpResponseLookupQuery =
    R"jsonString([{"api":"query","version":"v1","baseURL":"https://query.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";
const std::string kCacheKeyMetadata = kCatalog + "::query::v1::api";

const std::string kOlpSdkUrlPartitionByIdBase =
    R"(https://query.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layers/)" +
    kVersionedLayerId + R"(/partitions)";

const std::string kOlpSdkUrlPartitionByIdNoVersion =
    kOlpSdkUrlPartitionByIdBase + R"(?partition=)" + kPartitionId;

const std::string kOlpSdkUrlPartitionById = kOlpSdkUrlPartitionByIdNoVersion +
                                            R"(&version=)" +
                                            std::to_string(kVersion);

const std::string kOlpSdkUrlPartitionByIdWithAdditionalParams =
    kOlpSdkUrlPartitionByIdBase + R"(?additionalFields=)" +
    olp::utils::Url::Encode(R"(checksum,compressedDataSize,crc,dataSize)") +
    R"(&partition=)" + kPartitionId + R"(&version=)" + std::to_string(kVersion);

const std::string kOlpSdkHttpResponsePartitionById =
    R"jsonString({ "partitions": [{"version":42,"partition":")jsonString" +
    kPartitionId +
    R"jsonString(","layer":"olp-cpp-sdk-ingestion-test-volatile-layer","dataHandle":"PartitionsRepositoryTest-partitionId"}]})jsonString";

const std::string kOlpSdkHttpResponsePartitionByIdWithAdditionalFields =
    R"jsonString({ "partitions": [{"version":42,"partition":")jsonString" +
    kPartitionId +
    R"jsonString(","layer":"olp-cpp-sdk-ingestion-test-volatile-layer","dataHandle":"PartitionsRepositoryTest-partitionId","checksum":"xxx","compressedDataSize":15,"dataSize":10,"crc":"yyy"}]})jsonString";

const std::string kOlpSdkHttpResponseEmptyPartitionList =
    R"({ "partitions": [] })";

const std::string kOlpSdkUrlLookupConfig =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/platform/apis)";

const std::string kOlpSdkHttpResponseLookupConfig =
    R"jsonString([{"api":"config","version":"v1","baseURL":"https://config.data.api.platform.sit.here.com/config/v1","parameters":{}},{"api":"pipelines","version":"v1","baseURL":"https://pipelines.api.platform.sit.here.com/pipeline-service","parameters":{}},{"api":"pipelines","version":"v2","baseURL":"https://pipelines.api.platform.sit.here.com/pipeline-service","parameters":{}}])jsonString";

const std::string kOlpSdkUrlConfig =
    R"(https://config.data.api.platform.sit.here.com/config/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2)";

const std::string
    kOlpSdkHttpResponseConfig =
        R"jsonString({"id":"hereos-internal-test","hrn":"hrn:here-dev:data:::hereos-internal-test","name":"hereos-internal-test","summary":"Internal test for hereos","description":"Used for internal testing on the staging olp.","contacts":{},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"tags":[],"billingTags":[],"created":"2018-07-13T20:50:08.425Z","layers":[{"id":"hype-test-prefetch","hrn":"hrn:here-dev:data:::hereos-internal-test:hype-test-prefetch","name":"Hype Test Prefetch","summary":"hype prefetch testing","description":"Layer for hype prefetch testing","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":[],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer_res","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer_res","name":"Resource Test Layer","summary":"testlayer_res","description":"testlayer_res","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer_volatile","ttl":1000,"hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"volatile"},{"id":"testlayer_stream","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"stream"},{"id":"multilevel_testlayer","hrn":"hrn:here-dev:data:::hereos-internal-test:multilevel_testlayer","name":"Multi Level Test Layer","summary":"Multi Level Test Layer","description":"A multi level test layer just for testing","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"hype-test-prefetch-2","hrn":"hrn:here-dev:data:::hereos-internal-test:hype-test-prefetch-2","name":"Hype Test Prefetch2","summary":"Layer for testing hype2 prefetching","description":"Layer for testing hype2 prefetching","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-24T17:52:23.818Z","layerType":"versioned"}],"version":3})jsonString";

const std::string kOlpSdkUrlLookupMetadata2 =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data::olp-here-test:hereos-internal-test-v2/apis)";

const std::string kOlpSdkHttpResponseLookupMetadata2 =
    R"jsonString([{"api":"metadata","version":"v1","baseURL":"https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";

const std::string kOlpSdkUrlVersionedPartitions =
    R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layers/)" +
    kVersionedLayerId + R"(/partitions?version=)" + std::to_string(kVersion);

const std::string kOlpSdkUrlVolatilePartitions =
    R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layers/)" +
    kVolatileLayerId + R"(/partitions)";

const std::string kOlpSdkHttpResponsePartitions =
    R"jsonString({ "partitions": [{"version":100,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"},{"version":100,"partition":"270","layer":"testlayer","dataHandle":"30640762-b429-47b9-9ed6-7a4af6086e8e"},{"version":100,"partition":"3","layer":"testlayer","dataHandle":"data:SomethingBaH!"},{"version":100,"partition":"here_van_wc2018_pool","layer":"testlayer","dataHandle":"bcde4cc0-2678-40e9-b791-c630faee14c3"}]})jsonString";

const std::string kUrlLookupQuery =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data::olp-here-test:hereos-internal-test-v2/apis)";

const std::string kHttpResponseLookupQuery =
    R"jsonString([{"api":"query","version":"v1","baseURL":"https://sab.query.data.api.platform.here.com/query/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2","parameters":{}}])jsonString";

const std::string kUrlQueryApi =
    R"(https://sab.query.data.api.platform.here.com/query/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2)";

const std::string kQueryTreeIndex =
    R"(https://sab.query.data.api.platform.here.com/query/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/versions/100/quadkeys/23064/depths/4)";

const std::string kQueryTreeIndexWithAdditionalFields =
    kQueryTreeIndex + R"(?additionalFields=)" +
    olp::utils::Url::Encode(R"(checksum,crc,dataSize,compressedDataSize)");

const std::string kQueryQuadTreeIndex =
    R"(https://sab.query.data.api.platform.here.com/query/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/versions/100/quadkeys/90/depths/4)";

const std::string kSubQuads =
    R"jsonString({"subQuads": [{"subQuadKey":"115","version":100,"dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1"},{"subQuadKey":"463","version":100,"dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1"}],"parentQuads": []})jsonString";

const std::string kInvalidJson =
    R"jsonString({}"subQuads": [{"subQuadKey":"115","version":100,"dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1"},{"subQuadKey":"463","version":100,"dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1"}],"parentQuads": []})jsonString";

const std::string kSubQuadsWithParent =
    R"jsonString({"subQuads": [{"subQuadKey":"115","version":100,"dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1"},{"subQuadKey":"463","version":100,"dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1"}],"parentQuads": [{"partition":"5","version":282,"dataHandle":"13E2C624E0136C3357D092EE7F231E87.282","dataSize":99151}]})jsonString";

const std::string kSubQuadsWithParentAndAdditionalFields =
    R"jsonString({"subQuads": [{"subQuadKey":"115","version":100,"dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1","checksum":"xxx","compressedDataSize":10,"dataSize":15,"crc":"aaa"},{"subQuadKey":"463","version":100,"dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1","checksum":"yyy","compressedDataSize":20,"dataSize":25,"crc":"bbb"}],"parentQuads": [{"partition":"5","version":282,"dataHandle":"13E2C624E0136C3357D092EE7F231E87.282","checksum":"zzz","compressedDataSize":30,"dataSize":35,"crc":"ccc"}]})jsonString";

const std::string kSubQuadsWithParentAndAdditionalFieldsWithoutCrc =
    R"jsonString({"subQuads": [{"subQuadKey":"115","version":100,"dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1","checksum":"xxx","compressedDataSize":10,"dataSize":15},{"subQuadKey":"463","version":100,"dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1","checksum":"yyy","compressedDataSize":20,"dataSize":25}],"parentQuads": [{"partition":"5","version":282,"dataHandle":"13E2C624E0136C3357D092EE7F231E87.282","checksum":"zzz","compressedDataSize":30,"dataSize":35}]})jsonString";

const std::string kBlobDataHandle1476147 =
    R"(95c5c703-e00e-4c38-841e-e419367474f1)";

const std::string kErrorServiceUnavailable = "Service unavailable";

class PartitionsRepositoryTest : public ::testing::Test {};

TEST_F(PartitionsRepositoryTest, GetPartitionById) {
  using testing::Eq;
  using testing::Mock;
  using testing::Return;

  auto cache = std::make_shared<testing::StrictMock<CacheMock>>();
  auto network = std::make_shared<testing::StrictMock<NetworkMock>>();

  const auto catalog_hrn = HRN::FromString(kCatalog);

  OlpClientSettings settings;
  settings.cache = cache;
  settings.network_request_handler = network;
  settings.retry_settings.timeout = 1;

  ApiLookupClient lookup_client(catalog_hrn, settings);
  repository::PartitionsRepository repository(catalog_hrn, kVersionedLayerId,
                                              settings, lookup_client);

  const DataRequest request{DataRequest().WithPartitionId(kPartitionId)};
  const std::string part_cache_key =
      kCatalog + "::" + kVersionedLayerId + "::" + kPartitionId + "::";

  const std::string cache_key_no_version = part_cache_key + "partition";
  const std::string cache_key =
      part_cache_key + std::to_string(kVersion) + "::partition";

  auto setup_online_only_mocks = [&]() {
    ON_CALL(*cache, Get(_, _))
        .WillByDefault([](const std::string&, const cache::Decoder&) {
          ADD_FAILURE() << "Cache should not be used in OnlineOnly request";
          return boost::any{};
        });
  };

  auto setup_positive_metadata_mocks = [&]() {
    EXPECT_CALL(*network, Send(IsGetRequest(kOlpSdkUrlLookupQuery), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kOlpSdkHttpResponseLookupQuery));

    EXPECT_CALL(*cache, Put(Eq(kCacheKeyMetadata), _, _, _)).Times(0);
  };

  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] positive");

    const std::string query_cache_response =
        R"jsonString({"version":100,"partition":"1111","layer":"testlayer","dataHandle":"qwerty"})jsonString";

    EXPECT_CALL(*cache, Get(cache_key, _))
        .WillOnce(
            Return(parser::parse<model::Partition>(query_cache_response)));

    client::CancellationContext context;
    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::CacheOnly), kVersion,
        context);

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
        .WillOnce(Return(boost::any()));

    client::CancellationContext context;
    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::CacheOnly), kVersion,
        context);

    ASSERT_FALSE(response.IsSuccessful());
    const auto& result = response.GetError();
    EXPECT_EQ(result.GetErrorCode(), ErrorCode::NotFound);

    Mock::VerifyAndClearExpectations(cache.get());
  }
  {
    SCOPED_TRACE("Fetch with missing partition id");

    client::CancellationContext context;
    auto response = repository.GetPartitionById(
        DataRequest(request).WithPartitionId(boost::none), kVersion, context);

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

    EXPECT_CALL(*cache, Put(Eq(cache_key), _, _, _)).Times(0);

    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::OnlineOnly), kVersion,
        context);

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
    EXPECT_CALL(*cache, Put(Eq(cache_key_no_version), _, _, _)).Times(0);

    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::OnlineOnly), boost::none,
        context);

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

    EXPECT_CALL(*network, Send(IsGetRequest(kOlpSdkUrlLookupQuery), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::UNAUTHORIZED),
                               "Inappropriate"));

    client::CancellationContext context;
    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::OnlineOnly), kVersion,
        context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::AccessDenied);
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
    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::OnlineOnly), kVersion,
        context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::AccessDenied);
    Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE(
        "Network error 403 clears cache and is propagated to the user");
    setup_online_only_mocks();
    setup_positive_metadata_mocks();
    EXPECT_CALL(*cache, Get(cache_key, _))
        .WillOnce(Return(boost::any()));

    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlPartitionById), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::FORBIDDEN),
                                     "{Inappropriate}"));

    client::CancellationContext context;
    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::OnlineOnly), kVersion,
        context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::AccessDenied);
    Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE(
        "Network request cancelled by network internally at lookup state");
    setup_online_only_mocks();

    client::CancellationContext context;
    EXPECT_CALL(*network, Send(IsGetRequest(kOlpSdkUrlLookupQuery), _, _, _, _))
        .WillOnce([=](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback /*callback*/,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/)
                      -> olp::http::SendOutcome {
          return olp::http::SendOutcome(olp::http::ErrorCode::CANCELLED_ERROR);
        });

    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::OnlineOnly), kVersion,
        context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
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
        .WillOnce([=](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback /*callback*/,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/)
                      -> olp::http::SendOutcome {
          return olp::http::SendOutcome(olp::http::ErrorCode::CANCELLED_ERROR);
        });

    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::OnlineOnly), kVersion,
        context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE("Network request timed out at lookup state");
    setup_online_only_mocks();

    client::CancellationContext context;
    EXPECT_CALL(*network, Send(IsGetRequest(kOlpSdkUrlLookupQuery), _, _, _, _))
        .WillOnce([=](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback /*callback*/,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/)
                      -> olp::http::SendOutcome {
          // note no network response thread spawns
          constexpr auto unused_request_id = 12;
          return olp::http::SendOutcome(unused_request_id);
        });
    EXPECT_CALL(*network, Cancel(_)).WillOnce(Return());

    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::OnlineOnly), kVersion,
        context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::RequestTimeout);
    Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE("Network request timed out at partition state");
    setup_online_only_mocks();
    setup_positive_metadata_mocks();

    client::CancellationContext context;
    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlPartitionById), _, _, _, _))
        .WillOnce([=](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback /*callback*/,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/)
                      -> olp::http::SendOutcome {
          // note no network response thread spawns
          constexpr auto unused_request_id = 12;
          return olp::http::SendOutcome(unused_request_id);
        });
    EXPECT_CALL(*network, Cancel(_)).WillOnce(Return());

    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::OnlineOnly), kVersion,
        context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::RequestTimeout);
    Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE("Network request cancelled by user at lookup state");
    setup_online_only_mocks();

    client::CancellationContext context;
    EXPECT_CALL(*network, Send(IsGetRequest(kOlpSdkUrlLookupQuery), _, _, _, _))
        .WillOnce([=, &context](
                      olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback /*callback*/,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/)
                      -> olp::http::SendOutcome {
          // spawn a 'user' response of cancelling
          std::thread([&context]() { context.CancelOperation(); }).detach();

          // note no network response thread spawns

          constexpr auto unused_request_id = 12;
          return olp::http::SendOutcome(unused_request_id);
        });
    EXPECT_CALL(*network, Cancel(_)).WillOnce(Return());

    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::OnlineOnly), kVersion,
        context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Network request cancelled by user at patition state");
    setup_online_only_mocks();
    setup_positive_metadata_mocks();

    client::CancellationContext context;
    EXPECT_CALL(*network,
                Send(IsGetRequest(kOlpSdkUrlPartitionById), _, _, _, _))

        .WillOnce([=, &context](
                      olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback /*callback*/,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/)
                      -> olp::http::SendOutcome {
          // spawn a 'user' response of cancelling
          std::thread([&context]() { context.CancelOperation(); }).detach();

          // note no network response thread spawns

          constexpr auto unused_request_id = 12;
          return olp::http::SendOutcome(unused_request_id);
        });
    EXPECT_CALL(*network, Cancel(_)).WillOnce(Return());

    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::OnlineOnly), kVersion,
        context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE("Network request cancelled before execution setup");
    setup_online_only_mocks();

    client::CancellationContext context;
    context.CancelOperation();
    auto response = repository.GetPartitionById(
        DataRequest(request).WithFetchOption(read::OnlineOnly), kVersion,
        context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network.get());
  }
}

TEST_F(PartitionsRepositoryTest, GetVersionedPartitions) {
  using testing::Return;

  std::shared_ptr<cache::KeyValueCache> default_cache =
      client::OlpClientSettingsFactory::CreateDefaultCache({});

  auto mock_network = std::make_shared<NetworkMock>();
  auto cache = std::make_shared<testing::StrictMock<CacheMock>>();
  const auto catalog = HRN::FromString(kCatalog);

  {
    SCOPED_TRACE(
        "Fail the cache look up when one of the partitions is missing");
    OlpClientSettings settings;
    settings.cache = cache;
    settings.network_request_handler = mock_network;
    settings.retry_settings.timeout = 1;

    const std::string cache_key_1 =
        kCatalog + "::" + kVersionedLayerId + "::" + kPartitionId +
        "::" + std::to_string(kVersion) + "::partition";

    const std::string cache_key_2 =
        kCatalog + "::" + kVersionedLayerId + "::" + kInvalidPartitionId +
        "::" + std::to_string(kVersion) + "::partition";

    const std::string query_cache_response =
        R"jsonString({"version":100,"partition":"1111","layer":"testlayer","dataHandle":"qwerty"})jsonString";

    EXPECT_CALL(*cache, Get(cache_key_1, _))

        .WillOnce(
            Return(parser::parse<model::Partition>(query_cache_response)));

    EXPECT_CALL(*cache, Get(cache_key_2, _))

        .WillOnce(Return(boost::any()));

    client::CancellationContext context;
    ApiLookupClient lookup_client(catalog, settings);
    repository::PartitionsRepository repository(catalog, kVersionedLayerId,
                                                settings, lookup_client);

    read::PartitionsRequest request;
    request.WithPartitionIds({kPartitionId, kInvalidPartitionId});
    request.WithFetchOption(read::CacheOnly);

    auto response =
        repository.GetVersionedPartitions(request, kVersion, context);

    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_TRUE(response.GetResult().GetPartitions().empty());
  }
  {
    SCOPED_TRACE("Successful fetch from network with a list of partitions");

    OlpClientSettings settings;
    settings.cache = default_cache;
    settings.network_request_handler = mock_network;
    settings.retry_settings.timeout = 1;

    EXPECT_CALL(*mock_network,
                Send(IsGetRequest(kOlpSdkUrlLookupQuery), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kOlpSdkHttpResponseLookupQuery));

    EXPECT_CALL(*mock_network,
                Send(IsGetRequest(kOlpSdkUrlPartitionById), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kOlpSdkHttpResponsePartitionById));

    client::CancellationContext context;
    ApiLookupClient lookup_client(catalog, settings);
    repository::PartitionsRepository repository(catalog, kVersionedLayerId,
                                                settings, lookup_client);
    read::PartitionsRequest request;
    request.WithPartitionIds({kPartitionId});

    auto response =
        repository.GetVersionedPartitions(request, kVersion, context);

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    EXPECT_EQ(response.GetResult().GetPartitions().size(), 1);
  }
  {
    SCOPED_TRACE("Successful fetch from network, empty layer");

    OlpClientSettings settings;
    settings.cache = default_cache;
    settings.network_request_handler = mock_network;
    settings.retry_settings.timeout = 1;

    EXPECT_CALL(*mock_network,
                Send(IsGetRequest(kOlpSdkUrlLookupMetadata2), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kOlpSdkHttpResponseLookupMetadata2));

    EXPECT_CALL(*mock_network,
                Send(IsGetRequest(kOlpSdkUrlVersionedPartitions), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kOlpSdkHttpResponseEmptyPartitionList));

    client::CancellationContext context;
    ApiLookupClient lookup_client(catalog, settings);
    repository::PartitionsRepository repository(catalog, kVersionedLayerId,
                                                settings, lookup_client);
    read::PartitionsRequest request;

    auto response =
        repository.GetVersionedPartitions(request, kVersion, context);

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    EXPECT_TRUE(response.GetResult().GetPartitions().empty());

    request.WithFetchOption(read::CacheOnly);

    response = repository.GetVersionedPartitions(request, kVersion, context);

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    EXPECT_TRUE(response.GetResult().GetPartitions().empty());
  }
}

TEST_F(PartitionsRepositoryTest, GetVolatilePartitions) {
  using testing::Return;

  std::shared_ptr<cache::KeyValueCache> default_cache =
      client::OlpClientSettingsFactory::CreateDefaultCache({});

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
              Send(IsGetRequest(kOlpSdkUrlVolatilePartitions), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kOlpSdkHttpResponsePartitions));

  {
    SCOPED_TRACE("Successful fetch from network");

    OlpClientSettings settings;
    settings.cache = default_cache;
    settings.network_request_handler = mock_network;
    settings.retry_settings.timeout = 1;

    client::CancellationContext context;
    ApiLookupClient lookup_client(catalog, settings);
    repository::PartitionsRepository repository(catalog, kVolatileLayerId,
                                                settings, lookup_client);
    read::PartitionsRequest request;

    auto response = repository.GetVolatilePartitions(request, context);

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    EXPECT_EQ(response.GetResult().GetPartitions().size(), 4);
  }

  {
    SCOPED_TRACE("Successful fetch from only cache");

    OlpClientSettings settings;
    settings.cache = default_cache;
    settings.retry_settings.timeout = 0;

    ApiLookupClient lookup_client(catalog, settings);
    repository::PartitionsRepository repository(catalog, kVolatileLayerId,
                                                settings, lookup_client);
    client::CancellationContext context;
    read::PartitionsRequest request;
    request.WithFetchOption(read::CacheOnly);

    auto cache_only_response =
        repository.GetVolatilePartitions(request, context);

    ASSERT_TRUE(cache_only_response.IsSuccessful())
        << cache_only_response.GetError().GetMessage();
    EXPECT_EQ(cache_only_response.GetResult().GetPartitions().size(), 4);
  }
}

TEST_F(PartitionsRepositoryTest, AdditionalFields) {
  std::shared_ptr<cache::KeyValueCache> default_cache =
      client::OlpClientSettingsFactory::CreateDefaultCache({});

  auto mock_network = std::make_shared<NetworkMock>();
  const auto catalog = HRN::FromString(kCatalog);

  EXPECT_CALL(*mock_network,
              Send(IsGetRequest(kOlpSdkUrlLookupQuery), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kOlpSdkHttpResponseLookupQuery));

  EXPECT_CALL(*mock_network,
              Send(IsGetRequest(kOlpSdkUrlPartitionByIdWithAdditionalParams), _,
                   _, _, _))
      .WillOnce(ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          kOlpSdkHttpResponsePartitionByIdWithAdditionalFields));

  OlpClientSettings settings;
  settings.cache = default_cache;
  settings.network_request_handler = mock_network;

  ApiLookupClient lookup_client(catalog, settings);
  repository::PartitionsRepository repository(catalog, kVersionedLayerId,
                                              settings, lookup_client);
  client::CancellationContext context;
  read::PartitionsRequest request;

  request.WithPartitionIds({kPartitionId});
  request.WithAdditionalFields({read::PartitionsRequest::kChecksum,
                                read::PartitionsRequest::kCompressedDataSize,
                                read::PartitionsRequest::kCrc,
                                read::PartitionsRequest::kDataSize});

  auto response = repository.GetVersionedPartitions(request, kVersion, context);

  ASSERT_TRUE(response.IsSuccessful());
  auto result = response.GetResult();
  auto partitions = result.GetPartitions();
  ASSERT_EQ(partitions.size(), 1);
  EXPECT_EQ(partitions[0].GetDataSize().value_or(0), 10);
  EXPECT_EQ(partitions[0].GetCompressedDataSize().value_or(0), 15);
  EXPECT_EQ(partitions[0].GetChecksum().value_or(""), "xxx");
  EXPECT_EQ(partitions[0].GetCrc().value_or(""), "yyy");

  request.WithFetchOption(read::CacheOnly);

  auto response_2 =
      repository.GetVersionedPartitions(request, kVersion, context);

  ASSERT_TRUE(response_2.IsSuccessful());

  auto cached_result = response_2.GetResult();
  auto cached_partitions = cached_result.GetPartitions();
  ASSERT_EQ(cached_partitions.size(), 1);

  EXPECT_EQ(partitions[0].GetDataSize(), cached_partitions[0].GetDataSize());
  EXPECT_EQ(partitions[0].GetCompressedDataSize(),
            cached_partitions[0].GetCompressedDataSize());
  EXPECT_EQ(partitions[0].GetChecksum(), cached_partitions[0].GetChecksum());
  EXPECT_EQ(partitions[0].GetCrc(), cached_partitions[0].GetCrc());
}

TEST_F(PartitionsRepositoryTest, CheckCashedPartitions) {
  std::shared_ptr<cache::KeyValueCache> default_cache =
      client::OlpClientSettingsFactory::CreateDefaultCache({});
  auto mock_network = std::make_shared<NetworkMock>();
  OlpClientSettings settings;
  settings.cache = default_cache;
  settings.network_request_handler = mock_network;
  settings.retry_settings.timeout = 1;

  EXPECT_CALL(*mock_network, Send(IsGetRequest(kUrlLookupQuery), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupQuery));

  EXPECT_CALL(*mock_network, Send(IsGetRequest(kQueryTreeIndex), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kSubQuads));

  const auto hrn = HRN::FromString(kCatalog);

  ApiLookupClient lookup_client(hrn, settings);
  repository::PartitionsRepository repository(hrn, kVersionedLayerId, settings,
                                              lookup_client);

  {
    SCOPED_TRACE("query partitions and store to cache");
    auto request =
        read::TileRequest().WithTileKey(TileKey::FromHereTile("5904591"));
    olp::client::CancellationContext context;

    auto response = repository.GetTile(request, kVersion, context);

    ASSERT_TRUE(response.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetDataHandle(),
              "e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1");
  }

  {
    SCOPED_TRACE(
        "Check if all partitions stored in cache, request another tile");
    olp::client::CancellationContext context;
    auto request = read::TileRequest()
                       .WithTileKey(TileKey::FromHereTile("1476147"))
                       .WithFetchOption(read::CacheOnly);

    auto response = repository.GetTile(request, kVersion, context);

    // check if partition was stored to cache
    ASSERT_TRUE(response.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetDataHandle(), kBlobDataHandle1476147);
  }
}

TEST_F(PartitionsRepositoryTest, GetAggregatedPartitionForVersionedTile) {
  using cache::KeyValueCache;
  using testing::Return;

  const auto hrn = HRN::FromString(kCatalog);

  {
    SCOPED_TRACE("Same tile");

    const auto tile_key = TileKey::FromHereTile("23247");
    const auto request = read::TileRequest().WithTileKey(tile_key);
    olp::client::CancellationContext context;

    auto mock_network = std::make_shared<NetworkMock>();
    auto mock_cache = std::make_shared<CacheMock>();

    OlpClientSettings settings;
    settings.cache = mock_cache;
    settings.network_request_handler = mock_network;

    EXPECT_CALL(*mock_network, Send(IsGetRequest(kUrlLookupQuery), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kHttpResponseLookupQuery));
    EXPECT_CALL(*mock_network,
                Send(IsGetRequest(kQueryQuadTreeIndex), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kSubQuads));
    EXPECT_CALL(*mock_cache, Get(_, _)).WillOnce(Return(boost::any()));
    EXPECT_CALL(*mock_cache, Put(_, _, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*mock_cache, Get(_))
        .WillRepeatedly(Return(KeyValueCache::ValueTypePtr()));
    EXPECT_CALL(*mock_cache, Put(_, _, _)).WillOnce(Return(true));

    ApiLookupClient lookup_client(hrn, settings);
    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    auto response = repository.GetAggregatedTile(request, kVersion, context);
    const auto& result = response.GetResult();

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_EQ(result.GetPartition(), tile_key.ToHereTile());
  }
  {
    SCOPED_TRACE("QuadTree is cached");

    const auto depth = 4;
    const auto tile_key = TileKey::FromHereTile("23247");
    const auto request = read::TileRequest().WithTileKey(tile_key);
    olp::client::CancellationContext context;

    auto mock_network = std::make_shared<NetworkMock>();
    auto mock_cache = std::make_shared<CacheMock>();

    OlpClientSettings settings;
    settings.cache = mock_cache;
    settings.network_request_handler = mock_network;

    auto ss = std::stringstream(kSubQuads);
    read::QuadTreeIndex quad_tree(tile_key.ChangedLevelBy(-depth), depth, ss);

    EXPECT_CALL(*mock_cache, Get(_)).WillOnce(Return(quad_tree.GetRawData()));

    ApiLookupClient lookup_client(hrn, settings);
    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    auto response = repository.GetAggregatedTile(request, kVersion, context);

    const auto& result = response.GetResult();

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_EQ(result.GetPartition(), tile_key.ToHereTile());
  }

  {
    SCOPED_TRACE("QueryApi is cached");

    const auto tile_key = TileKey::FromHereTile("23247");
    const auto request = read::TileRequest().WithTileKey(tile_key);
    olp::client::CancellationContext context;

    auto mock_network = std::make_shared<NetworkMock>();
    auto mock_cache = std::make_shared<CacheMock>();

    OlpClientSettings settings;
    settings.cache = mock_cache;
    settings.network_request_handler = mock_network;

    EXPECT_CALL(*mock_network,
                Send(IsGetRequest(kQueryQuadTreeIndex), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kSubQuadsWithParent));
    EXPECT_CALL(*mock_cache, Get(_, _))
        .WillOnce(Return(boost::any(kUrlQueryApi)));
    EXPECT_CALL(*mock_cache, Get(_))
        .WillRepeatedly(Return(KeyValueCache::ValueTypePtr()));
    EXPECT_CALL(*mock_cache, Put(_, _, _)).WillOnce(Return(true));

    ApiLookupClient lookup_client(hrn, settings);
    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    auto response = repository.GetAggregatedTile(request, kVersion, context);

    const auto& result = response.GetResult();

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_EQ(result.GetPartition(), tile_key.ToHereTile());
  }

  {
    SCOPED_TRACE("No tiles found");

    const auto tile_key = TileKey::FromHereTile("23064");
    const auto request = read::TileRequest().WithTileKey(tile_key);
    olp::client::CancellationContext context;

    auto mock_network = std::make_shared<NetworkMock>();
    auto mock_cache = std::make_shared<CacheMock>();

    OlpClientSettings settings;
    settings.cache = mock_cache;
    settings.network_request_handler = mock_network;

    EXPECT_CALL(*mock_network, Send(IsGetRequest(kUrlLookupQuery), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kHttpResponseLookupQuery));
    EXPECT_CALL(*mock_network,
                Send(IsGetRequest(kQueryQuadTreeIndex), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kSubQuads));
    EXPECT_CALL(*mock_cache, Get(_, _)).WillOnce(Return(boost::any()));
    EXPECT_CALL(*mock_cache, Put(_, _, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*mock_cache, Get(_))
        .WillRepeatedly(Return(KeyValueCache::ValueTypePtr()));
    EXPECT_CALL(*mock_cache, Put(_, _, _)).WillOnce(Return(true));

    ApiLookupClient lookup_client(hrn, settings);
    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    auto response = repository.GetAggregatedTile(request, kVersion, context);

    const auto& error = response.GetError();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(error.GetErrorCode(), ErrorCode::NotFound);
  }

  {
    SCOPED_TRACE("CacheOnly");

    const auto tile_key = TileKey::FromHereTile("23064");
    const auto request =
        read::TileRequest().WithTileKey(tile_key).WithFetchOption(
            read::CacheOnly);
    olp::client::CancellationContext context;

    auto mock_network = std::make_shared<NetworkMock>();
    auto mock_cache = std::make_shared<CacheMock>();

    OlpClientSettings settings;
    settings.cache = mock_cache;
    settings.network_request_handler = mock_network;

    EXPECT_CALL(*mock_cache, Get(_))
        .WillRepeatedly(Return(KeyValueCache::ValueTypePtr()));

    ApiLookupClient lookup_client(hrn, settings);
    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    auto response = repository.GetAggregatedTile(request, kVersion, context);
    const auto& error = response.GetError();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(error.GetErrorCode(), ErrorCode::NotFound);
  }

  {
    SCOPED_TRACE("QueryApi request failed");

    const auto tile_key = TileKey::FromHereTile("23247");
    const auto request = read::TileRequest().WithTileKey(tile_key);
    olp::client::CancellationContext context;

    auto mock_network = std::make_shared<NetworkMock>();
    auto mock_cache = std::make_shared<CacheMock>();

    OlpClientSettings settings;
    settings.cache = mock_cache;
    settings.network_request_handler = mock_network;

    EXPECT_CALL(*mock_network, Send(IsGetRequest(kUrlLookupQuery), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               kErrorServiceUnavailable));
    EXPECT_CALL(*mock_cache, Get(_, _)).WillOnce(Return(boost::any()));
    EXPECT_CALL(*mock_cache, Get(_))
        .WillRepeatedly(Return(KeyValueCache::ValueTypePtr()));

    ApiLookupClient lookup_client(hrn, settings);
    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    auto response = repository.GetAggregatedTile(request, kVersion, context);

    const auto& error = response.GetError();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(error.GetHttpStatusCode(),
              olp::http::HttpStatusCode::BAD_REQUEST);
    ASSERT_EQ(error.GetMessage(), kErrorServiceUnavailable);
  }

  {
    SCOPED_TRACE("QuadTreeIndex request failed");

    const auto tile_key = TileKey::FromHereTile("23247");
    const auto request = read::TileRequest().WithTileKey(tile_key);
    olp::client::CancellationContext context;

    auto mock_network = std::make_shared<NetworkMock>();
    auto mock_cache = std::make_shared<CacheMock>();

    OlpClientSettings settings;
    settings.cache = mock_cache;
    settings.network_request_handler = mock_network;

    EXPECT_CALL(*mock_network, Send(IsGetRequest(kUrlLookupQuery), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kHttpResponseLookupQuery));
    EXPECT_CALL(*mock_network,
                Send(IsGetRequest(kQueryQuadTreeIndex), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               kErrorServiceUnavailable));
    EXPECT_CALL(*mock_cache, Get(_, _)).WillOnce(Return(boost::any()));
    EXPECT_CALL(*mock_cache, Put(_, _, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*mock_cache, Get(_))
        .WillRepeatedly(Return(KeyValueCache::ValueTypePtr()));

    ApiLookupClient lookup_client(hrn, settings);
    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    auto response = repository.GetAggregatedTile(request, kVersion, context);

    const auto& error = response.GetError();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(error.GetHttpStatusCode(),
              olp::http::HttpStatusCode::BAD_REQUEST);
    ASSERT_EQ(error.GetMessage(), kErrorServiceUnavailable);
  }

  {
    SCOPED_TRACE("Failed to parse json");

    const auto tile_key = TileKey::FromHereTile("23247");
    const auto request = read::TileRequest().WithTileKey(tile_key);
    olp::client::CancellationContext context;

    auto mock_network = std::make_shared<NetworkMock>();
    auto mock_cache = std::make_shared<CacheMock>();

    OlpClientSettings settings;
    settings.cache = mock_cache;
    settings.network_request_handler = mock_network;

    EXPECT_CALL(*mock_network, Send(IsGetRequest(kUrlLookupQuery), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kHttpResponseLookupQuery));
    EXPECT_CALL(*mock_network,
                Send(IsGetRequest(kQueryQuadTreeIndex), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kInvalidJson));
    EXPECT_CALL(*mock_cache, Get(_, _)).WillOnce(Return(boost::any()));
    EXPECT_CALL(*mock_cache, Put(_, _, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*mock_cache, Get(_))
        .WillRepeatedly(Return(KeyValueCache::ValueTypePtr()));

    ApiLookupClient lookup_client(hrn, settings);
    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    auto response = repository.GetAggregatedTile(request, kVersion, context);
    const auto& error = response.GetError();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(error.GetErrorCode(), ErrorCode::Unknown);
  }
}

TEST_F(PartitionsRepositoryTest, GetTile) {
  using testing::Mock;
  using testing::Return;

  const auto hrn = HRN::FromString(kCatalog);
  olp::client::CancellationContext context;

  auto mock_network = std::make_shared<NetworkMock>();
  auto mock_cache = std::make_shared<CacheMock>();

  OlpClientSettings settings;
  settings.cache = mock_cache;
  settings.network_request_handler = mock_network;

  const auto depth = 4;
  auto quad_cache_key = [&](const TileKey& key) {
    return kCatalog + "::" + kVersionedLayerId + "::" + key.ToHereTile() +
           "::" + std::to_string(kVersion) + "::" + std::to_string(depth) +
           "::quadtree";
  };

  std::string expected_api = kCatalog + "::query::v1::api";
  ApiLookupClient lookup_client(hrn, settings);
  EXPECT_CALL(*mock_cache, Get(expected_api, _)).WillOnce(Return(kUrlQueryApi));

  auto tile_key = TileKey::FromHereTile("23064");
  auto root = tile_key.ChangedLevelBy(-depth);
  auto request = read::TileRequest().WithTileKey(tile_key);

  const auto setup_get_cached_quad_expectations =
      [&](const boost::optional<std::string>& root_data = boost::none) {
        testing::InSequence sequence;

        for (auto i = 0; i < depth; ++i) {
          EXPECT_CALL(*mock_cache,
                      Get(quad_cache_key(tile_key.ChangedLevelBy(-i))))
              .WillOnce(Return(nullptr));
        }

        EXPECT_CALL(*mock_cache, Get(quad_cache_key(root)))
            .WillOnce(testing::WithoutArgs(
                [=]() -> cache::KeyValueCache::ValueTypePtr {
                  if (!root_data) {
                    return nullptr;
                  }

                  auto stream = std::stringstream(*root_data);
                  read::QuadTreeIndex quad_tree(root, depth, stream);
                  return quad_tree.GetRawData();
                }));
      };

  {
    SCOPED_TRACE("Get tile not aggregated, partition not found");

    setup_get_cached_quad_expectations();
    EXPECT_CALL(*mock_network,
                Send(IsGetRequest(kQueryQuadTreeIndex), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kSubQuadsWithParent));
    EXPECT_CALL(*mock_cache, Put(quad_cache_key(root), _, _))
        .WillOnce(Return(true));

    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    const auto response = repository.GetTile(request, kVersion, context);

    ASSERT_FALSE(response);

    testing::Mock::VerifyAndClearExpectations(mock_network.get());
    testing::Mock::VerifyAndClearExpectations(mock_cache.get());
  }

  const auto kHereTile = "5904591";
  tile_key = TileKey::FromHereTile(kHereTile);
  root = tile_key.ChangedLevelBy(-depth);
  request = read::TileRequest().WithTileKey(tile_key);

  {
    SCOPED_TRACE("Get tile not aggregated");

    setup_get_cached_quad_expectations();
    EXPECT_CALL(*mock_network, Send(IsGetRequest(kQueryTreeIndex), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kSubQuadsWithParent));
    EXPECT_CALL(*mock_cache, Put(quad_cache_key(root), _, _))
        .WillOnce(Return(true));

    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    const auto response = repository.GetTile(request, kVersion, context);

    ASSERT_TRUE(response);
    EXPECT_EQ(response.GetResult().GetPartition(), kHereTile);

    testing::Mock::VerifyAndClearExpectations(mock_network.get());
    testing::Mock::VerifyAndClearExpectations(mock_cache.get());
  }

  std::vector<std::string> all_additional_fields = {
      read::PartitionsRequest::kChecksum, read::PartitionsRequest::kCrc,
      read::PartitionsRequest::kDataSize,
      read::PartitionsRequest::kCompressedDataSize};

  // Expected values
  int64_t data_size = 25;
  int64_t compressed_data_size = 20;
  std::string checksum = "yyy";
  std::string crc = "bbb";

  {
    SCOPED_TRACE("Get tile not aggregated with additional fields");

    setup_get_cached_quad_expectations();
    EXPECT_CALL(
        *mock_network,
        Send(IsGetRequest(kQueryTreeIndexWithAdditionalFields), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kSubQuadsWithParentAndAdditionalFields));
    EXPECT_CALL(*mock_cache, Put(quad_cache_key(root), _, _))
        .WillOnce(Return(true));

    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    const auto response =
        repository.GetTile(request, kVersion, context, all_additional_fields);

    ASSERT_TRUE(response);
    const auto& result = response.GetResult();
    EXPECT_EQ(result.GetPartition(), kHereTile);
    EXPECT_TRUE(result.GetDataSize());
    EXPECT_EQ(*result.GetDataSize(), data_size);
    EXPECT_TRUE(result.GetCompressedDataSize());
    EXPECT_EQ(*result.GetCompressedDataSize(), compressed_data_size);
    EXPECT_TRUE(result.GetChecksum());
    EXPECT_EQ(*result.GetChecksum(), checksum);
    EXPECT_TRUE(result.GetCrc());
    EXPECT_EQ(*result.GetCrc(), crc);

    testing::Mock::VerifyAndClearExpectations(mock_network.get());
    testing::Mock::VerifyAndClearExpectations(mock_cache.get());
  }

  {
    SCOPED_TRACE(
        "Cached partition without additional fields, request without "
        "additional fields");

    setup_get_cached_quad_expectations(kSubQuadsWithParent);
    EXPECT_CALL(*mock_network, Send(_, _, _, _, _)).Times(0);
    EXPECT_CALL(*mock_cache, Put(_, _, _)).Times(0);

    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    const auto response = repository.GetTile(request, kVersion, context);

    ASSERT_TRUE(response);
    const auto& result = response.GetResult();
    EXPECT_EQ(result.GetPartition(), kHereTile);
    EXPECT_FALSE(result.GetDataSize());
    EXPECT_FALSE(result.GetCompressedDataSize());
    EXPECT_FALSE(result.GetChecksum());
    EXPECT_FALSE(result.GetCrc());

    testing::Mock::VerifyAndClearExpectations(mock_network.get());
    testing::Mock::VerifyAndClearExpectations(mock_cache.get());
  }

  {
    SCOPED_TRACE(
        "Cached partition with additional fields, request without additional "
        "fields");

    setup_get_cached_quad_expectations(kSubQuadsWithParentAndAdditionalFields);
    EXPECT_CALL(*mock_network, Send(_, _, _, _, _)).Times(0);
    EXPECT_CALL(*mock_cache, Put(_, _, _)).Times(0);

    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    const auto response = repository.GetTile(request, kVersion, context);

    ASSERT_TRUE(response);
    const auto& result = response.GetResult();
    EXPECT_EQ(result.GetPartition(), kHereTile);
    EXPECT_TRUE(result.GetDataSize());
    EXPECT_EQ(*result.GetDataSize(), data_size);
    EXPECT_TRUE(result.GetCompressedDataSize());
    EXPECT_EQ(*result.GetCompressedDataSize(), compressed_data_size);
    EXPECT_TRUE(result.GetChecksum());
    EXPECT_EQ(*result.GetChecksum(), checksum);
    EXPECT_TRUE(result.GetCrc());
    EXPECT_EQ(*result.GetCrc(), crc);

    testing::Mock::VerifyAndClearExpectations(mock_network.get());
    testing::Mock::VerifyAndClearExpectations(mock_cache.get());
  }

  {
    SCOPED_TRACE(
        "Cached partition without additional fields, request with additional "
        "fields");

    setup_get_cached_quad_expectations(kSubQuadsWithParent);
    EXPECT_CALL(
        *mock_network,
        Send(IsGetRequest(kQueryTreeIndexWithAdditionalFields), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kSubQuadsWithParentAndAdditionalFields));
    EXPECT_CALL(*mock_cache, Put(quad_cache_key(root), _, _))
        .WillOnce(Return(true));

    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    const auto response =
        repository.GetTile(request, kVersion, context, all_additional_fields);

    ASSERT_TRUE(response);
    const auto& result = response.GetResult();
    EXPECT_EQ(result.GetPartition(), kHereTile);
    EXPECT_TRUE(result.GetDataSize());
    EXPECT_EQ(*result.GetDataSize(), data_size);
    EXPECT_TRUE(result.GetCompressedDataSize());
    EXPECT_EQ(*result.GetCompressedDataSize(), compressed_data_size);
    EXPECT_TRUE(result.GetChecksum());
    EXPECT_EQ(*result.GetChecksum(), checksum);
    EXPECT_TRUE(result.GetCrc());
    EXPECT_EQ(*result.GetCrc(), crc);

    testing::Mock::VerifyAndClearExpectations(mock_network.get());
    testing::Mock::VerifyAndClearExpectations(mock_cache.get());
  }

  {
    SCOPED_TRACE(
        "Cached partition with not all additional fields, request with all "
        "additional fields");

    setup_get_cached_quad_expectations(
        kSubQuadsWithParentAndAdditionalFieldsWithoutCrc);
    EXPECT_CALL(
        *mock_network,
        Send(IsGetRequest(kQueryTreeIndexWithAdditionalFields), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kSubQuadsWithParentAndAdditionalFields));
    EXPECT_CALL(*mock_cache, Put(quad_cache_key(root), _, _))
        .WillOnce(Return(true));

    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    const auto response =
        repository.GetTile(request, kVersion, context, all_additional_fields);

    ASSERT_TRUE(response);
    const auto& result = response.GetResult();
    EXPECT_EQ(result.GetPartition(), kHereTile);
    EXPECT_TRUE(result.GetDataSize());
    EXPECT_EQ(*result.GetDataSize(), data_size);
    EXPECT_TRUE(result.GetCompressedDataSize());
    EXPECT_EQ(*result.GetCompressedDataSize(), compressed_data_size);
    EXPECT_TRUE(result.GetChecksum());
    EXPECT_EQ(*result.GetChecksum(), checksum);
    EXPECT_TRUE(result.GetCrc());
    EXPECT_EQ(*result.GetCrc(), crc);

    testing::Mock::VerifyAndClearExpectations(mock_network.get());
    testing::Mock::VerifyAndClearExpectations(mock_cache.get());
  }

  {
    SCOPED_TRACE(
        "Cached partition with additional fields, request with additional "
        "fields");

    setup_get_cached_quad_expectations(kSubQuadsWithParentAndAdditionalFields);
    EXPECT_CALL(*mock_network, Send(_, _, _, _, _)).Times(0);
    EXPECT_CALL(*mock_cache, Put(_, _, _)).Times(0);

    repository::PartitionsRepository repository(hrn, kVersionedLayerId,
                                                settings, lookup_client);
    const auto response =
        repository.GetTile(request, kVersion, context, all_additional_fields);

    ASSERT_TRUE(response);
    const auto& result = response.GetResult();
    EXPECT_EQ(result.GetPartition(), kHereTile);
    EXPECT_TRUE(result.GetDataSize());
    EXPECT_EQ(*result.GetDataSize(), data_size);
    EXPECT_TRUE(result.GetCompressedDataSize());
    EXPECT_EQ(*result.GetCompressedDataSize(), compressed_data_size);
    EXPECT_TRUE(result.GetChecksum());
    EXPECT_EQ(*result.GetChecksum(), checksum);
    EXPECT_TRUE(result.GetCrc());
    EXPECT_EQ(*result.GetCrc(), crc);

    testing::Mock::VerifyAndClearExpectations(mock_network.get());
    testing::Mock::VerifyAndClearExpectations(mock_cache.get());
  }
}
}  // namespace
