/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include <algorithm>
#include <chrono>
#include <memory>
#include <numeric>
#include <string>
#include <thread>

#include <matchers/NetworkUrlMatchers.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include <olp/authentication/Settings.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyGenerator.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/porting/warning_disable.h>
#include <olp/core/utils/Dir.h>
#include <olp/core/utils/Url.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
#include <olp/dataservice/read/model/Partitions.h>

#include "HttpResponses.h"

using olp::dataservice::read::AggregatedDataResponse;
using olp::dataservice::read::DataResponse;
using olp::dataservice::read::FetchOptions;
using olp::dataservice::read::PartitionsRequest;
using olp::dataservice::read::PartitionsResponse;
using olp::dataservice::read::PrefetchTilesRequest;
using olp::dataservice::read::PrefetchTilesResponse;
using testing::_;

namespace {

namespace read = olp::dataservice::read;
namespace client = olp::client;
namespace http = olp::http;
namespace geo = olp::geo;

/// This method is used by URL_CONFIG macro
std::string GetTestCatalog() {
  return "hrn:here:data::olp-here-test:hereos-internal-test-v2";
}

const client::HRN kCatalog = client::HRN::FromString(GetTestCatalog());
constexpr auto kTestLayer = "testlayer";
constexpr auto kTestPartition = "269";
constexpr auto kTestVersion = 108;

std::string ApiErrorToString(const client::ApiError& error) {
  std::ostringstream result_stream;
  result_stream << "ERROR: code: " << static_cast<int>(error.GetErrorCode())
                << ", status: " << error.GetHttpStatusCode()
                << ", message: " << error.GetMessage();
  return result_stream.str();
}

constexpr char kHttpResponsePartition_269[] =
    R"jsonString({ "partitions": [{"version":4,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"}]})jsonString";

constexpr char kHttpResponsePartitionsEmpty[] =
    R"jsonString({ "partitions": []})jsonString";

constexpr char kHttpResponseBlobData_269[] = R"jsonString(DT_2_0031)jsonString";

constexpr char kHttpResponseLatestCatalogVersion[] =
    R"jsonString({"version":4})jsonString";

constexpr char kHttpResponseBlobData_5904591[] =
    R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1)";

constexpr char kHttpQueryTreeIndex_23064[] =
    R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/versions/4/quadkeys/23064/depths/4)";

const auto kHttpQueryTreeIndexWithAdditionalFields_23064 =
    std::string(kHttpQueryTreeIndex_23064) +
    "?additionalFields=" + olp::utils::Url::Encode("checksum,crc,dataSize");

constexpr char kHttpSubQuads_23064[] =
    R"jsonString({"subQuads": [{"subQuadKey":"115","version":4,"dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1"},{"subQuadKey":"463","version":4,"dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1"}],"parentQuads": []})jsonString";

constexpr char kHttpSubQuadsWithAdditionalFields_23064[] =
    R"jsonString({"subQuads": [{"subQuadKey":"115","version":4,"dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1","checksum":"xxx","dataSize":10,"crc":"aaa"},{"subQuadKey":"463","version":4,"dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1","checksum":"yyy","dataSize":20,"crc":"bbb"}],"parentQuads": []})jsonString";

constexpr char kUrlBlobData_1476147[] =
    R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/95c5c703-e00e-4c38-841e-e419367474f1)";

constexpr auto kUrlPartitionsPrefix =
    R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions)";

constexpr auto kUrlBlobstorePrefix =
    R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/)";

constexpr auto kWaitTimeout = std::chrono::seconds(3);

class DataserviceReadVersionedLayerClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    network_mock_ = std::make_shared<NetworkMock>();

    settings_.network_request_handler = network_mock_;
    settings_.task_scheduler =
        client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
    settings_.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});

    SetUpCommonNetworkMockCalls();
  }

  void TearDown() override {
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
    network_mock_.reset();
    settings_.task_scheduler.reset();
  }

  void SetUpCommonNetworkMockCalls() {
    ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_LOOKUP_CONFIG));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_CONFIG));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_LOOKUP));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                               HTTP_RESPONSE_LATEST_CATALOG_VERSION));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_LAYER_VERSIONS), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_LAYER_VERSIONS));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_PARTITIONS));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_LOOKUP));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_LAYER_VERSIONS_V2), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_LAYER_VERSIONS_V2));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS_V10), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_LAYER_VERSIONS_V2));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS_VN1), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_INVALID_VERSION_VN1));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_PARTITIONS_INVALID_LAYER), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_INVALID_LAYER));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS_V2), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_PARTITIONS_V2));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_LAYER_VERSIONS_V10), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_INVALID_VERSION_V10));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_LAYER_VERSIONS_VN1), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_INVALID_VERSION_VN1));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_QUADKEYS_1476147), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_QUADKEYS_1476147));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_QUADKEYS_92259));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_1), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_BLOB_DATA_PREFETCH_1));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_2), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_BLOB_DATA_PREFETCH_2));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_4), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_BLOB_DATA_PREFETCH_4));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_5), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_BLOB_DATA_PREFETCH_5));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_6), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_BLOB_DATA_PREFETCH_6));
    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_7), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_BLOB_DATA_PREFETCH_7));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_BLOB_DATA_269));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_PARTITION_269));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_QUERY_PARTITION_269_V2), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_PARTITION_269_V2));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_269_V2), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                          HTTP_RESPONSE_BLOB_DATA_269_V2));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_QUERY_PARTITION_269_V10), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_INVALID_VERSION_V10));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_QUERY_PARTITION_269_VN1), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_INVALID_VERSION_VN1));

    // Catch any non-interesting network calls that don't need to be verified
    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _)).Times(testing::AtLeast(0));
  }

 protected:
  client::OlpClientSettings settings_;
  std::shared_ptr<NetworkMock> network_mock_;
};

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataFromPartitionAsync) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(
          ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK, 1024, 123),
                             kHttpResponsePartition_269))
      .WillOnce(
          ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK, 1024, 123),
                             kHttpResponseBlobData_269));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, kTestVersion, settings_);

  auto promise = std::make_shared<std::promise<DataResponse>>();
  std::future<DataResponse> future = promise->get_future();

  auto token = client->GetData(
      read::DataRequest().WithPartitionId(kTestPartition),
      [promise](DataResponse response) { promise->set_value(response); });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_NE(response.GetResult(), nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);

  const auto& network_stats = response.GetPayload();
  EXPECT_EQ(network_stats.GetBytesDownloaded(), 2048);
  EXPECT_EQ(network_stats.GetBytesUploaded(), 246);

  const auto cache_key = olp::cache::KeyGenerator::CreatePartitionKey(
      GetTestCatalog(), kTestLayer, kTestPartition, kTestVersion);
  EXPECT_TRUE(settings_.cache->Contains(cache_key));
}

template <typename T>
std::function<void(T)> placeholder(std::promise<T>& promise) {
  return [&promise](T result) { promise.set_value(result); };
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataPartitionConcurrent) {
  const std::chrono::milliseconds request_delay(100);

  // Setup the network mock to expect necessary call exact once
  {
    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP, {}, request_delay))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kHttpResponsePartition_269, {},
                                     request_delay))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kHttpResponseBlobData_269, {},
                                     request_delay));
  }

  const auto concurrent_calls = 5;

  // Replace the default task_scheduler, since it uses only 1 thread
  settings_.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(
          concurrent_calls);

  read::VersionedLayerClient client(kCatalog, kTestLayer, kTestVersion,
                                    settings_);

  std::vector<std::promise<read::DataResponse>> promises(concurrent_calls);

  for (auto& promise : promises) {
    client.GetData(read::DataRequest().WithPartitionId(kTestPartition),
                   placeholder(promise));
  }

  for (auto& promise : promises) {
    auto response = promise.get_future().get();

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_NE(response.GetResult(), nullptr);
    ASSERT_NE(response.GetResult()->size(), 0u);
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionAsyncWithCancellableFuture) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, kTestVersion, settings_);

  auto data_request = read::DataRequest().WithPartitionId(kTestPartition);
  auto cancellable_future = client->GetData(std::move(data_request));

  auto raw_future = cancellable_future.GetFuture();
  ASSERT_NE(raw_future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = raw_future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_NE(response.GetResult(), nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataFromPartitionSync) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto sync_settings = settings_;
  sync_settings.task_scheduler.reset();
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 0, sync_settings);

  DataResponse response;

  auto token = client->GetData(
      read::DataRequest().WithPartitionId(kTestPartition),
      [&response](DataResponse resp) { response = std::move(resp); });
  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionSyncWithCancellableFuture) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto sync_settings = settings_;
  sync_settings.task_scheduler.reset();
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 0, sync_settings);

  auto data_request = read::DataRequest().WithPartitionId(kTestPartition);
  auto cancellable_future = client->GetData(std::move(data_request));

  auto raw_future = cancellable_future.GetFuture();
  ASSERT_NE(raw_future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = raw_future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_NE(response.GetResult(), nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionSyncLatestVersionOk) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponseLatestCatalogVersion))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto sync_settings = settings_;
  sync_settings.task_scheduler.reset();
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, sync_settings);

  DataResponse response;

  auto token = client->GetData(
      read::DataRequest().WithPartitionId(kTestPartition),
      [&response](DataResponse resp) { response = std::move(resp); });
  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionSyncLatestVersionInvalid) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::FORBIDDEN),
                                   kHttpResponseLatestCatalogVersion));

  auto sync_settings = settings_;
  sync_settings.task_scheduler.reset();
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, sync_settings);

  DataResponse response;

  auto token = client->GetData(
      read::DataRequest().WithPartitionId(kTestPartition),
      [&response](DataResponse resp) { response = std::move(resp); });
  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_FALSE(response.GetResult() != nullptr);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionOnlineIfNotFoundCacheUpdate) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto sync_settings = settings_;
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 269, sync_settings);

  std::promise<DataResponse> promise;

  client->GetData(
      read::DataRequest()
          .WithPartitionId(kTestPartition)
          .WithFetchOption(FetchOptions::OnlineIfNotFound),
      [&promise](DataResponse resp) { promise.set_value(std::move(resp)); });

  auto future = promise.get_future();
  auto response = future.get();

  ASSERT_TRUE(response.IsSuccessful());

  promise = std::promise<DataResponse>();
  client->GetData(
      read::DataRequest()
          .WithPartitionId(kTestPartition)
          .WithFetchOption(FetchOptions::CacheOnly),
      [&promise](DataResponse resp) { promise.set_value(std::move(resp)); });

  future = promise.get_future();
  response = future.get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataEmptyPartitionsSync) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponsePartitionsEmpty));

  auto sync_settings = settings_;
  sync_settings.task_scheduler.reset();
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 0, sync_settings);

  DataResponse response;

  auto token = client->GetData(
      read::DataRequest().WithPartitionId(kTestPartition),
      [&response](DataResponse resp) { response = std::move(resp); });
  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_FALSE(response.GetResult() != nullptr);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionCancelLookup) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, kTestVersion, settings_);

  std::promise<DataResponse> promise;
  std::future<DataResponse> future = promise.get_future();

  auto token = client->GetData(
      read::DataRequest().WithPartitionId(kTestPartition),
      [&promise](DataResponse response) { promise.set_value(response); });

  wait_for_cancel->get_future().get();
  token.Cancel();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_TRUE(response.GetResult() == nullptr);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionCancelLookupWithCancellableFuture) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, kTestVersion, settings_);

  auto data_request = read::DataRequest().WithPartitionId(kTestPartition);

  auto cancellable_future = client->GetData(std::move(data_request));

  wait_for_cancel->get_future().get();
  cancellable_future.GetCancellationToken().Cancel();
  pause_for_cancel->set_value();

  auto raw_future = cancellable_future.GetFuture();
  ASSERT_NE(raw_future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = raw_future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_TRUE(response.GetResult() == nullptr);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionCancelPartition) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, kHttpResponsePartition_269});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, kTestVersion, settings_);

  std::promise<DataResponse> promise;
  std::future<DataResponse> future = promise.get_future();

  auto token = client->GetData(
      read::DataRequest().WithPartitionId(kTestPartition),
      [&promise](DataResponse response) { promise.set_value(response); });

  wait_for_cancel->get_future().get();
  token.Cancel();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_TRUE(response.GetResult() == nullptr);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionCancelLookupBlob) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, kTestVersion, settings_);

  std::promise<DataResponse> promise;
  std::future<DataResponse> future = promise.get_future();

  auto token = client->GetData(
      read::DataRequest().WithPartitionId(kTestPartition),
      [&promise](DataResponse response) { promise.set_value(response); });

  wait_for_cancel->get_future().get();
  token.Cancel();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_TRUE(response.GetResult() == nullptr);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionCancelBlobData) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, kHttpResponseBlobData_269});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, kTestVersion, settings_);

  std::promise<DataResponse> promise;
  std::future<DataResponse> future = promise.get_future();

  auto token = client->GetData(
      read::DataRequest()
          .WithPartitionId(kTestPartition)
          .WithFetchOption(FetchOptions::OnlineIfNotFound),
      [&promise](DataResponse response) { promise.set_value(response); });

  wait_for_cancel->get_future().get();
  token.Cancel();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_TRUE(response.GetResult() == nullptr);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataPriority) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP));
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kHttpQueryTreeIndex_23064), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpSubQuads_23064));
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kHttpResponseBlobData_5904591), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   "someData"));

  constexpr auto version = 4;
  auto scheduler = settings_.task_scheduler;
  std::promise<void> block_promise;
  std::promise<void> finish_promise;
  auto block_future = block_promise.get_future();
  auto finish_future = finish_promise.get_future();

  scheduler->ScheduleTask([&]() { block_future.wait_for(kWaitTimeout); },
                          std::numeric_limits<uint32_t>::max());

  auto priority = 700u;
  // this priority should be less than `priority`, but greater than NORMAL
  auto finish_task_priority = 600u;

  auto client =
      read::VersionedLayerClient(kCatalog, kTestLayer, version, settings_);

  auto request = read::TileRequest()
                     .WithTileKey(geo::TileKey::FromHereTile("5904591"))
                     .WithPriority(priority);
  auto future = client.GetData(request).GetFuture();
  scheduler->ScheduleTask(
      [&]() {
        EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)),
                  std::future_status::ready);
        finish_promise.set_value();
      },
      finish_task_priority);

  // unblock queue
  block_promise.set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  ASSERT_NE(finish_future.wait_for(kWaitTimeout), std::future_status::timeout);

  auto response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_NE(response.GetResult(), nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest, DataRequestPriority) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto scheduler = settings_.task_scheduler;
  std::promise<void> block_promise;
  std::promise<void> finish_promise;
  auto block_future = block_promise.get_future();
  auto finish_future = finish_promise.get_future();

  scheduler->ScheduleTask([&]() { block_future.wait_for(kWaitTimeout); },
                          std::numeric_limits<uint32_t>::max());

  auto priority = 700u;
  // this priority should be less than `priority`, but greater than NORMAL
  auto finish_task_priority = 600u;

  auto client =
      read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion, settings_);

  auto request = read::DataRequest()
                     .WithPartitionId(kTestPartition)
                     .WithPriority(priority);
  auto future = client.GetData(request).GetFuture();
  scheduler->ScheduleTask(
      [&]() {
        EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)),
                  std::future_status::ready);
        finish_promise.set_value();
      },
      finish_task_priority);

  // unblock queue
  block_promise.set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  ASSERT_NE(finish_future.wait_for(kWaitTimeout), std::future_status::timeout);

  auto response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_NE(response.GetResult(), nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsNoError) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::PartitionsRequest();
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
  auto future = promise->get_future();
  auto token = client->GetPartitions(
      request,
      [promise](PartitionsResponse response) { promise->set_value(response); });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(4u, response.GetResult().GetPartitions().size());

  const auto version = 4;
  const auto cache_key = olp::cache::KeyGenerator::CreatePartitionsKey(
      GetTestCatalog(), kTestLayer, version);
  EXPECT_TRUE(settings_.cache->Contains(cache_key));
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetPartitionsCancellableFutureNoError) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::PartitionsRequest();
  auto cancellable_future = client->GetPartitions(request);
  auto future = cancellable_future.GetFuture();
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(4u, response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetPartitionsCancellableFutureCancellation) {
  settings_.task_scheduler->ScheduleTask(
      []() { std::this_thread::sleep_for(std::chrono::seconds(1)); });

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::PartitionsRequest();
  auto cancellable_future = client->GetPartitions(request);
  auto future = cancellable_future.GetFuture();

  cancellable_future.GetCancellationToken().Cancel();

  ASSERT_EQ(std::future_status::ready, future.wait_for(kWaitTimeout));

  auto response = future.get();
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetEmptyPartitions) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_EMPTY_PARTITIONS));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::PartitionsRequest();
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
  auto future = promise->get_future();
  auto token = client->GetPartitions(
      request,
      [promise](PartitionsResponse response) { promise->set_value(response); });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(0u, response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitions429Error) {
  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .Times(2)
        .WillRepeatedly(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .Times(1);
  }

  client::RetrySettings retry_settings;
  retry_settings.retry_condition = [](const client::HttpResponse& response) {
    return http::HttpStatusCode::TOO_MANY_REQUESTS == response.status;
  };
  settings_.retry_settings = retry_settings;
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::PartitionsRequest();
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
  auto future = promise->get_future();
  auto token = client->GetPartitions(
      request,
      [promise](PartitionsResponse response) { promise->set_value(response); });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(4u, response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVersionedLayerClientTest, ApiLookup429) {
  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(2)
        .WillRepeatedly(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);
  }

  client::RetrySettings retry_settings;
  retry_settings.retry_condition = [](const client::HttpResponse& response) {
    return http::HttpStatusCode::TOO_MANY_REQUESTS == response.status;
  };
  settings_.retry_settings = retry_settings;
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::PartitionsRequest();
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
  auto future = promise->get_future();
  auto token = client->GetPartitions(
      request,
      [promise](PartitionsResponse response) { promise->set_value(response); });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(4u, response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsForInvalidLayer) {
  const auto layer = "somewhat_not_okay";

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, layer, boost::none, settings_);

  auto request = read::PartitionsRequest();
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
  auto future = promise->get_future();
  auto token = client->GetPartitions(
      request,
      [promise](PartitionsResponse response) { promise->set_value(response); });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(client::ErrorCode::BadRequest, response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsCacheWithUpdate) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  // Request 1
  {
    auto promise = std::make_shared<std::promise<PartitionsResponse>>();
    auto future = promise->get_future();
    auto request =
        PartitionsRequest().WithFetchOption(FetchOptions::CacheWithUpdate);
    auto token =
        client->GetPartitions(request, [promise](PartitionsResponse response) {
          promise->set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PartitionsResponse response = future.get();
    // Request 1 return. Cached value (nothing)
    ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  }

  // Request 2
  {
    auto promise = std::make_shared<std::promise<PartitionsResponse>>();
    auto future = promise->get_future();
    auto request = PartitionsRequest().WithFetchOption(FetchOptions::CacheOnly);
    auto token =
        client->GetPartitions(request, [promise](PartitionsResponse response) {
          promise->set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PartitionsResponse response = future.get();
    // Cache should not be available for versioned layer.
    ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitions403CacheClear) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);
  {
    testing::InSequence s;
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::FORBIDDEN), HTTP_RESPONSE_403));
  }

  // Populate cache
  auto request = read::PartitionsRequest();

  {
    auto promise = std::make_shared<std::promise<PartitionsResponse>>();
    auto future = promise->get_future();
    auto token =
        client->GetPartitions(request, [promise](PartitionsResponse response) {
          promise->set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PartitionsResponse response = future.get();

    ASSERT_TRUE(response.IsSuccessful());
  }

  // Receive 403
  {
    request.WithFetchOption(FetchOptions::OnlineOnly);

    auto promise = std::make_shared<std::promise<PartitionsResponse>>();
    auto future = promise->get_future();
    auto token =
        client->GetPartitions(request, [promise](PartitionsResponse response) {
          promise->set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PartitionsResponse response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(http::HttpStatusCode::FORBIDDEN,
              response.GetError().GetHttpStatusCode());
  }

  // Check for cached response
  {
    request.WithFetchOption(FetchOptions::CacheOnly);
    auto promise = std::make_shared<std::promise<PartitionsResponse>>();
    auto future = promise->get_future();
    auto token =
        client->GetPartitions(request, [promise](PartitionsResponse response) {
          promise->set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PartitionsResponse response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsGarbageResponse) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   R"jsonString(kd3sdf\)jsonString"));

  auto request = read::PartitionsRequest();
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
  auto future = promise->get_future();
  auto token = client->GetPartitions(
      request,
      [promise](PartitionsResponse response) { promise->set_value(response); });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(client::ErrorCode::Unknown, response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetPartitionsCancelLookupMetadata) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  auto request = read::PartitionsRequest();
  std::promise<PartitionsResponse> promise;
  auto future = promise.get_future();
  auto token = client->GetPartitions(
      request,
      [&promise](PartitionsResponse response) { promise.set_value(response); });

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
            response.GetError().GetHttpStatusCode())
      << response.GetError().GetMessage();
  ASSERT_EQ(client::ErrorCode::Cancelled, response.GetError().GetErrorCode())
      << response.GetError().GetMessage();
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetPartitionsCancelLatestCatalogVersion) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LATEST_CATALOG_VERSION});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LAYER_VERSIONS), _, _, _, _))
      .Times(0);

  auto request = read::PartitionsRequest();
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
  auto future = promise->get_future();
  auto token = client->GetPartitions(
      request,
      [promise](PartitionsResponse response) { promise->set_value(response); });

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
            response.GetError().GetHttpStatusCode())
      << response.GetError().GetMessage();
  ASSERT_EQ(client::ErrorCode::Cancelled, response.GetError().GetErrorCode())
      << response.GetError().GetMessage();
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetPartitionsCancelLayerVersions) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LAYER_VERSIONS});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(0);

  auto request = read::PartitionsRequest();
  std::promise<PartitionsResponse> promise;
  auto future = promise.get_future();
  auto token = client->GetPartitions(
      request,
      [&promise](PartitionsResponse response) { promise.set_value(response); });

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
            response.GetError().GetHttpStatusCode())
      << response.GetError().GetMessage();
  ASSERT_EQ(client::ErrorCode::Cancelled, response.GetError().GetErrorCode())
      << response.GetError().GetMessage();
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsVersion2) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 2, settings_);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS_V2), _, _, _, _))
      .Times(1);

  auto request = read::PartitionsRequest();
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
  auto future = promise->get_future();
  auto token = client->GetPartitions(
      request,
      [promise](PartitionsResponse response) { promise->set_value(response); });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(1u, response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsInvalidVersion) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 10, settings_);

  auto request = read::PartitionsRequest();
  {
    auto promise = std::make_shared<std::promise<PartitionsResponse>>();
    auto future = promise->get_future();
    auto token =
        client->GetPartitions(request, [promise](PartitionsResponse response) {
          promise->set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PartitionsResponse response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(client::ErrorCode::BadRequest,
              response.GetError().GetErrorCode());
    ASSERT_EQ(http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
  }

  {
    auto promise = std::make_shared<std::promise<PartitionsResponse>>();
    auto future = promise->get_future();
    auto token =
        client->GetPartitions(request, [promise](PartitionsResponse response) {
          promise->set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PartitionsResponse response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(client::ErrorCode::BadRequest,
              response.GetError().GetErrorCode());
    ASSERT_EQ(http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsCacheOnly) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(0);

  auto request = read::PartitionsRequest();
  request.WithFetchOption(FetchOptions::CacheOnly);
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
  auto future = promise->get_future();
  auto token = client->GetPartitions(
      request,
      [promise](PartitionsResponse response) { promise->set_value(response); });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsOnlineOnly) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(4)
        .WillRepeatedly(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));
  }

  auto request = read::PartitionsRequest();
  {
    auto promise = std::make_shared<std::promise<PartitionsResponse>>();
    auto future = promise->get_future();
    auto token =
        client->GetPartitions(request, [promise](PartitionsResponse response) {
          promise->set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PartitionsResponse response = future.get();

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_EQ(4u, response.GetResult().GetPartitions().size());
  }

  {
    request.WithFetchOption(FetchOptions::OnlineOnly);
    auto promise = std::make_shared<std::promise<PartitionsResponse>>();
    auto future = promise->get_future();
    auto token =
        client->GetPartitions(request, [promise](PartitionsResponse response) {
          promise->set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PartitionsResponse response = future.get();

    // Should fail despite valid cache entry
    ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, PrefetchTilesWithCache) {
  constexpr auto kLayerId = "hype-test-prefetch";

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kLayerId, boost::none, settings_);

  {
    SCOPED_TRACE("Prefetch tiles online and store them in memory cache");
    std::vector<geo::TileKey> tile_keys = {
        geo::TileKey::FromHereTile("5904591")};

    auto request = read::PrefetchTilesRequest()
                       .WithTileKeys(tile_keys)
                       .WithMinLevel(11)
                       .WithMaxLevel(12);

    auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
    auto future = promise->get_future();
    auto token = client->PrefetchTiles(
        request, [promise](PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PrefetchTilesResponse response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();
    ASSERT_EQ(result.size(), 5);

    for (auto tile_result : result) {
      ASSERT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }

    const auto tree_root = olp::geo::TileKey::FromQuadKey64(92259);
    const auto version = 4;
    const auto depth = 4;
    const auto cache_key = olp::cache::KeyGenerator::CreateQuadTreeKey(
        GetTestCatalog(), kLayerId, tree_root, version, depth);
    EXPECT_TRUE(settings_.cache->Get(cache_key));
  }

  {
    SCOPED_TRACE("Read cached data from pre-fetched sub-partition #1");
    auto promise = std::make_shared<std::promise<DataResponse>>();
    std::future<DataResponse> future = promise->get_future();
    auto token =
        client->GetData(read::TileRequest()
                            .WithTileKey(geo::TileKey::FromHereTile("23618365"))
                            .WithFetchOption(FetchOptions::CacheOnly),
                        [promise](DataResponse response) {
                          promise->set_value(std::move(response));
                        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);

    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << ApiErrorToString(response.GetError());
    ASSERT_TRUE(response.GetResult() != nullptr);
    ASSERT_NE(response.GetResult()->size(), 0u);
  }

  {
    SCOPED_TRACE("Read cached data from pre-fetched sub-partition #2");
    auto promise = std::make_shared<std::promise<DataResponse>>();
    std::future<DataResponse> future = promise->get_future();

    auto token =
        client->GetData(read::TileRequest()
                            .WithTileKey(geo::TileKey::FromHereTile("23618366"))
                            .WithFetchOption(FetchOptions::CacheOnly),
                        [promise](DataResponse response) {
                          promise->set_value(std::move(response));
                        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);

    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << ApiErrorToString(response.GetError());
    ASSERT_TRUE(response.GetResult() != nullptr);
    ASSERT_NE(response.GetResult()->size(), 0u);
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       PrefetchTilesWithRootTilesInBetweenLevels) {
  constexpr auto kLayerId = "hype-test-prefetch";

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kLayerId, boost::none, settings_);

  {
    SCOPED_TRACE("Prefetch tiles where tile greater min level");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_1), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_6), _, _, _, _))
        .Times(1);
    std::vector<geo::TileKey> tile_keys = {
        geo::TileKey::FromHereTile("23618366")};

    auto request = read::PrefetchTilesRequest()
                       .WithTileKeys(tile_keys)
                       .WithMinLevel(11)
                       .WithMaxLevel(12);

    auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
    auto future = promise->get_future();
    auto token = client->PrefetchTiles(
        request, [promise](PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PrefetchTilesResponse response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();
    ASSERT_EQ(result.size(),
              2);  // parent tile and root tile in PrefetchRequest

    for (auto tile_result : result) {
      ASSERT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }

  {
    SCOPED_TRACE("Prefetch tiles with cache");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_1), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_6), _, _, _, _))
        .Times(0);
    std::vector<geo::TileKey> tile_keys = {
        geo::TileKey::FromHereTile("23618366")};

    auto request = read::PrefetchTilesRequest()
                       .WithTileKeys(tile_keys)
                       .WithMinLevel(11)
                       .WithMaxLevel(12);

    auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
    auto future = promise->get_future();
    auto token = client->PrefetchTiles(
        request, [promise](PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PrefetchTilesResponse response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();
    ASSERT_EQ(result.size(),
              2);  // parent tile and root tile in PrefetchRequest

    for (auto tile_result : result) {
      ASSERT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       PrefetchSibilingTilesDefaultLevels) {
  constexpr auto kLayerId = "hype-test-prefetch";

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kLayerId, boost::none, settings_);

  {
    SCOPED_TRACE("Prefetch tiles online, ");
    std::vector<geo::TileKey> tile_keys = {
        geo::TileKey::FromHereTile("23618366"),
        geo::TileKey::FromHereTile("23618365")};

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_QUADKEYS_92259));
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_23618364), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_1476147), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_5904591), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_369036), _, _, _, _))
        .Times(0);

    auto request = read::PrefetchTilesRequest().WithTileKeys(tile_keys);

    auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
    auto future = promise->get_future();
    auto token = client->PrefetchTiles(
        request, [promise](PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PrefetchTilesResponse response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();

    for (auto tile_result : result) {
      ASSERT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       PrefetchAggregatedTile_FilterTileKeysByList) {
  constexpr auto kLayerId = "hype-test-prefetch";

  const auto requested_tile = geo::TileKey::FromHereTile("23618365");

  read::VersionedLayerClient client(kCatalog, kLayerId, boost::none, settings_);

  {
    SCOPED_TRACE("Prefetch aggregated tile");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_QUADKEYS_92259_ROOT_ONLY));

    auto request = read::PrefetchTilesRequest()
                       .WithTileKeys({requested_tile})
                       .WithDataAggregationEnabled(true);

    auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
    auto future = promise->get_future();
    auto token = client.PrefetchTiles(
        request, [promise](PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PrefetchTilesResponse response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();
    ASSERT_EQ(result.size(), 1);

    const auto& tile_response = result[0];
    ASSERT_TRUE(tile_response->IsSuccessful());
    ASSERT_TRUE(tile_response->tile_key_.IsParentOf(requested_tile));
  }
  {
    SCOPED_TRACE("Check that the tile is available as aggregated");
    EXPECT_TRUE(client.IsCached(requested_tile, true));
    EXPECT_FALSE(client.IsCached(requested_tile));
  }
  {
    SCOPED_TRACE("Check that the tile can be accesed with GetAggregatedData");
    auto future = client.GetAggregatedData(
        read::TileRequest().WithTileKey(requested_tile));
    auto result = future.GetFuture().get();
    EXPECT_TRUE(result.IsSuccessful());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       PrefetchAggregatedTile_FilterTileKeysByLevel) {
  constexpr auto kLayerId = "hype-test-prefetch";

  const auto requested_tile = geo::TileKey::FromHereTile("23618365");

  read::VersionedLayerClient client(kCatalog, kLayerId, boost::none, settings_);

  {
    SCOPED_TRACE("Prefetch aggregated tile");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_QUADKEYS_92259_ROOT_ONLY));

    auto request = read::PrefetchTilesRequest()
                       .WithTileKeys({requested_tile})
                       .WithMinLevel(11)
                       .WithDataAggregationEnabled(true);

    auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
    auto future = promise->get_future();
    auto token = client.PrefetchTiles(
        request, [promise](PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PrefetchTilesResponse response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();
    ASSERT_EQ(result.size(), 1);

    const auto& tile_response = result[0];
    ASSERT_TRUE(tile_response->IsSuccessful());
    ASSERT_TRUE(tile_response->tile_key_.IsParentOf(requested_tile));
  }
  {
    SCOPED_TRACE("Check that the tile is available as aggregated");
    EXPECT_TRUE(client.IsCached(requested_tile, true));
    EXPECT_FALSE(client.IsCached(requested_tile));
  }
  {
    SCOPED_TRACE("Check that the tile can be accesed with GetAggregatedData");
    auto future = client.GetAggregatedData(
        read::TileRequest().WithTileKey(requested_tile));
    auto result = future.GetFuture().get();
    EXPECT_TRUE(result.IsSuccessful());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, PrefetchTilesWrongLevels) {
  constexpr auto kLayerId = "hype-test-prefetch";

  std::vector<geo::TileKey> tile_keys = {geo::TileKey::FromHereTile("5904591")};

  ON_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillByDefault(ReturnHttpResponse(
          GetResponse(http::HttpStatusCode::FORBIDDEN), HTTP_RESPONSE_403));

  auto request = read::PrefetchTilesRequest().WithTileKeys(tile_keys);

  auto client = std::make_shared<read::VersionedLayerClient>(kCatalog, kLayerId,
                                                             4, settings_);

  auto cancel_future = client->PrefetchTiles(request);
  auto raw_future = cancel_future.GetFuture();

  ASSERT_NE(raw_future.wait_for(kWaitTimeout), std::future_status::timeout);
  PrefetchTilesResponse response = raw_future.get();
  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(client::ErrorCode::AccessDenied,
            response.GetError().GetErrorCode());
  ASSERT_TRUE(response.GetResult().empty());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       PrefetchTilesCancelOnClientDeletion) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  std::promise<PrefetchTilesResponse> promise;
  auto future = promise.get_future();

  constexpr auto kLayerId = "prefetch-catalog";
  constexpr auto kParitionId = "prefetch-partition";

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kLayerId, boost::none, settings_);

  std::vector<geo::TileKey> tile_keys = {
      geo::TileKey::FromHereTile(kParitionId)};
  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(11)
                     .WithMaxLevel(12);

  auto token = client->PrefetchTiles(
      request, [&promise](PrefetchTilesResponse response) {
        promise.set_value(std::move(response));
      });

  wait_for_cancel->get_future().get();
  client.reset();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PrefetchTilesResponse response = future.get();
  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
}

TEST_F(DataserviceReadVersionedLayerClientTest, PrefetchTilesCancelOnLookup) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  std::promise<PrefetchTilesResponse> promise;
  auto future = promise.get_future();

  constexpr auto kLayerId = "prefetch-catalog";
  constexpr auto kParitionId = "prefetch-partition";

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kLayerId, boost::none, settings_);

  std::vector<geo::TileKey> tile_keys = {
      geo::TileKey::FromHereTile(kParitionId)};
  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);

  auto token = client->PrefetchTiles(
      request, [&promise](PrefetchTilesResponse response) {
        promise.set_value(response);
      });

  wait_for_cancel->get_future().get();
  token.Cancel();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PrefetchTilesResponse response = future.get();
  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       PrefetchTilesWithCancellableFuture) {
  constexpr auto kLayerId = "hype-test-prefetch";

  std::vector<geo::TileKey> tile_keys = {geo::TileKey::FromHereTile("5904591")};

  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kLayerId, boost::none, settings_);

  auto cancel_future = client->PrefetchTiles(request);
  auto raw_future = cancel_future.GetFuture();

  ASSERT_NE(raw_future.wait_for(kWaitTimeout), std::future_status::timeout);
  PrefetchTilesResponse response = raw_future.get();
  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_FALSE(response.GetResult().empty());

  const auto& result = response.GetResult();
  for (auto tile_result : result) {
    ASSERT_TRUE(tile_result->IsSuccessful())
        << tile_result->GetError().GetMessage();
    ASSERT_TRUE(tile_result->tile_key_.IsValid());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       CancelPrefetchTilesWithCancellableFuture) {
  constexpr auto kLayerId = "hype-test-prefetch";

  std::vector<geo::TileKey> tile_keys = {geo::TileKey::FromHereTile("5904591")};

  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kLayerId, boost::none, settings_);

  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto cancel_future = client->PrefetchTiles(request);

  wait_for_cancel->get_future().get();
  cancel_future.GetCancellationToken().Cancel();
  pause_for_cancel->set_value();

  auto raw_future = cancel_future.GetFuture();
  ASSERT_NE(raw_future.wait_for(kWaitTimeout), std::future_status::timeout);
  PrefetchTilesResponse response = raw_future.get();
  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().empty());
}

TEST_F(DataserviceReadVersionedLayerClientTest, PrefetchTilesInvalidResponse) {
  constexpr auto kLayerId = "hype-test-prefetch";

  auto client =
      read::VersionedLayerClient(kCatalog, kLayerId, boost::none, settings_);
  std::vector<geo::TileKey> tile_keys = {geo::TileKey::FromHereTile("5904591")};

  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(11)
                     .WithMaxLevel(12);
  {
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     "some invalid json"));
  }
  auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
  auto future = promise->get_future();
  auto token =
      client.PrefetchTiles(request, [promise](PrefetchTilesResponse response) {
        promise->set_value(std::move(response));
      });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PrefetchTilesResponse response = future.get();
  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_TRUE(response.GetResult().empty());
  ASSERT_EQ(client::ErrorCode::Unknown, response.GetError().GetErrorCode());
  ASSERT_EQ("Failed to parse quad tree response",
            response.GetError().GetMessage());
}

TEST_F(DataserviceReadVersionedLayerClientTest, PrefetchTilesEmptyResponse) {
  constexpr auto kLayerId = "hype-test-prefetch";

  auto client =
      read::VersionedLayerClient(kCatalog, kLayerId, boost::none, settings_);
  std::vector<geo::TileKey> tile_keys = {
      geo::TileKey::FromHereTile("23618365")};

  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(11)
                     .WithMaxLevel(12);
  {
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::OK),
            R"jsonString({"subQuads": [],"parentQuads": []})jsonString"));
  }
  auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
  auto future = promise->get_future();
  auto token =
      client.PrefetchTiles(request, [promise](PrefetchTilesResponse response) {
        promise->set_value(std::move(response));
      });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PrefetchTilesResponse response = future.get();
  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_TRUE(response.GetResult().empty());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       PrefetchTilesEmptyResponseDefaultLevels) {
  constexpr auto kLayerId = "hype-test-prefetch";

  auto client =
      read::VersionedLayerClient(kCatalog, kLayerId, boost::none, settings_);
  std::vector<geo::TileKey> tile_keys = {
      geo::TileKey::FromHereTile("23618365")};

  auto request = read::PrefetchTilesRequest().WithTileKeys(tile_keys);
  {
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::OK),
            R"jsonString({"subQuads": [],"parentQuads": []})jsonString"));
  }
  auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
  auto future = promise->get_future();
  auto token =
      client.PrefetchTiles(request, [promise](PrefetchTilesResponse response) {
        promise->set_value(std::move(response));
      });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PrefetchTilesResponse response = future.get();
  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_FALSE(response.GetResult().empty());
  for (auto tile_result : response.GetResult()) {
    std::string str = tile_result->tile_key_.ToHereTile();
    ASSERT_FALSE(tile_result->IsSuccessful());
    ASSERT_EQ(olp::client::ErrorCode::NotFound,
              tile_result->GetError().GetErrorCode());
    ASSERT_TRUE(tile_result->tile_key_.IsValid());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, PrefetchSameTiles) {
  constexpr auto kLayerId = "hype-test-prefetch";

  auto client =
      read::VersionedLayerClient(kCatalog, kLayerId, boost::none, settings_);
  std::vector<geo::TileKey> tile_keys = {geo::TileKey::FromHereTile("5904591")};

  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(11)
                     .WithMaxLevel(12);
  {
    SCOPED_TRACE("Prefetch tiles online and store them in memory cache");
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_QUADKEYS_92259));

    auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
    auto future = promise->get_future();
    auto token = client.PrefetchTiles(
        request, [promise](PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PrefetchTilesResponse response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();

    for (auto tile_result : result) {
      ASSERT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }
  {
    SCOPED_TRACE("Prefetch same tiles second time");
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
        .Times(0);

    auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
    auto future = promise->get_future();
    auto token = client.PrefetchTiles(
        request, [promise](PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PrefetchTilesResponse response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();

    for (auto tile_result : result) {
      ASSERT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, PrefetchTilesWithStatus) {
  constexpr auto kLayerId = "hype-test-prefetch";

  auto client =
      read::VersionedLayerClient(kCatalog, kLayerId, boost::none, settings_);
  std::vector<geo::TileKey> tile_keys = {geo::TileKey::FromHereTile("5904591")};

  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(11)
                     .WithMaxLevel(12);
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK)
                                       .WithBytesDownloaded(200)
                                       .WithBytesUploaded(50),
                                   HTTP_RESPONSE_QUADKEYS_92259));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_6), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK)
                                       .WithBytesDownloaded(100)
                                       .WithBytesUploaded(50),
                                   HTTP_RESPONSE_BLOB_DATA_PREFETCH_6));

  struct Status {
    MOCK_METHOD(void, Op, (read::PrefetchStatus));
  };

  Status status_object;

  size_t bytes_transferred{0};

  {
    using testing::InSequence;
    using testing::Truly;

    auto matches = [](uint32_t prefetched, uint32_t total) {
      return [=](read::PrefetchStatus status) -> bool {
        return status.prefetched_tiles == prefetched &&
               status.total_tiles_to_prefetch == total;
      };
    };

    InSequence sequence;
    EXPECT_CALL(status_object, Op(Truly(matches(1, 5)))).Times(1);
    EXPECT_CALL(status_object, Op(Truly(matches(2, 5)))).Times(1);
    EXPECT_CALL(status_object, Op(Truly(matches(3, 5)))).Times(1);
    EXPECT_CALL(status_object, Op(Truly(matches(4, 5)))).Times(1);
    EXPECT_CALL(status_object, Op(Truly(matches(5, 5)))).Times(1);
  }

  auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
  auto future = promise->get_future();
  auto token = client.PrefetchTiles(request,
                                    [promise](PrefetchTilesResponse response) {
                                      promise->set_value(std::move(response));
                                    },
                                    [&](read::PrefetchStatus status) {
                                      status_object.Op(status);
                                      bytes_transferred =
                                          status.bytes_transferred;
                                    });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PrefetchTilesResponse response = future.get();
  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_FALSE(response.GetResult().empty());

  const auto& result = response.GetResult();

  for (auto tile_result : result) {
    ASSERT_TRUE(tile_result->IsSuccessful());
    ASSERT_TRUE(tile_result->tile_key_.IsValid());
  }

  EXPECT_GE(bytes_transferred, 400);

  testing::Mock::VerifyAndClearExpectations(&status_object);
}

TEST_F(DataserviceReadVersionedLayerClientTest, PrefetchPriority) {
  constexpr auto kLayerId = "hype-test-prefetch";
  auto scheduler = settings_.task_scheduler;
  std::promise<void> block_promise;
  std::promise<void> finish_promise;
  auto block_future = block_promise.get_future();
  auto finish_future = finish_promise.get_future();

  scheduler->ScheduleTask([&]() { block_future.wait_for(kWaitTimeout); },
                          std::numeric_limits<uint32_t>::max());

  auto client =
      read::VersionedLayerClient(kCatalog, kLayerId, boost::none, settings_);
  std::vector<geo::TileKey> tile_keys = {geo::TileKey::FromHereTile("5904591")};

  auto priority = 300u;
  // this priority should be less than priority, but greater than LOW
  auto finish_task_priority = 200u;

  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(11)
                     .WithMaxLevel(12)
                     .WithPriority(priority);
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_QUADKEYS_92259));

  auto future = client.PrefetchTiles(request).GetFuture();
  scheduler->ScheduleTask(
      [&]() {
        EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)),
                  std::future_status::ready);
        finish_promise.set_value();
      },
      finish_task_priority);

  // unblock queue
  block_promise.set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  ASSERT_NE(finish_future.wait_for(kWaitTimeout), std::future_status::timeout);

  PrefetchTilesResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_FALSE(response.GetResult().empty());

  const auto& result = response.GetResult();

  for (auto tile_result : result) {
    ASSERT_TRUE(tile_result->IsSuccessful());
    ASSERT_TRUE(tile_result->tile_key_.IsValid());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetData404Error) {
  EXPECT_CALL(
      *network_mock_,
      Send(IsGetRequest("https://blob-ireland.data.api.platform.here.com/"
                        "blobstore/v1/catalogs/hereos-internal-test-v2/"
                        "layers/testlayer/data/invalidDataHandle"),
           _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::NOT_FOUND),
                                   "Resource not found."));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::DataRequest();
  request.WithDataHandle("invalidDataHandle");
  auto future = client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(http::HttpStatusCode::NOT_FOUND,
            data_response.GetError().GetHttpStatusCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetData429Error) {
  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(2)
        .WillRepeatedly(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1);
  }

  client::RetrySettings retry_settings;
  retry_settings.retry_condition = [](const client::HttpResponse& response) {
    return http::HttpStatusCode::TOO_MANY_REQUESTS == response.status;
  };
  settings_.retry_settings = retry_settings;
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::DataRequest();
  request.WithDataHandle("4eed6ed1-0d32-43b9-ae79-043cb4256432");

  auto future = client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031", data_string);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetData403CacheClear) {
  {
    testing::InSequence s;
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::FORBIDDEN), HTTP_RESPONSE_403));
  }

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);
  auto request = read::DataRequest();
  request.WithPartitionId(kTestPartition);
  // Populate cache
  auto future = client->GetData(request);
  DataResponse data_response = future.GetFuture().get();
  ASSERT_TRUE(data_response.IsSuccessful());
  // Receive 403
  request.WithFetchOption(FetchOptions::OnlineOnly);
  future = client->GetData(request);
  data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(http::HttpStatusCode::FORBIDDEN,
            data_response.GetError().GetHttpStatusCode());
  // Check for cached response
  request.WithFetchOption(FetchOptions::CacheOnly);
  future = client->GetData(request);
  data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataCacheWithUpdate) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);
  auto request = read::DataRequest();
  request.WithPartitionId(kTestPartition)
      .WithFetchOption(FetchOptions::CacheWithUpdate);
  // Request 1
  auto future = client->GetData(request);
  DataResponse data_response = future.GetFuture().get();
  // Request 1 return. Cached value (nothing)
  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  // Request 2 to check there is a cached value.
  request.WithFetchOption(FetchOptions::CacheOnly);
  future = client->GetData(request);
  data_response = future.GetFuture().get();
  // Cache should not be available for versioned layer.
  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       CancelPendingRequestsPartitions) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);
  auto partitions_request =
      PartitionsRequest().WithFetchOption(FetchOptions::OnlineOnly);
  auto data_request = read::DataRequest()
                          .WithPartitionId(kTestPartition)
                          .WithFetchOption(FetchOptions::OnlineOnly);

  auto request_started = std::make_shared<std::promise<void>>();
  auto continue_request = std::make_shared<std::promise<void>>();

  {
    http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        request_started, continue_request,
        {http::HttpStatusCode::OK, HTTP_RESPONSE_BLOB_DATA_269});

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }

  auto data_future = client->GetData(data_request);
  auto partitions_future = client->GetPartitions(partitions_request);

  request_started->get_future().get();
  client->CancelPendingRequests();
  continue_request->set_value();

  PartitionsResponse partitions_response = partitions_future.GetFuture().get();

  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());

  ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
            partitions_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(client::ErrorCode::Cancelled,
            partitions_response.GetError().GetErrorCode());

  DataResponse data_response = data_future.GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());

  ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, CancelPendingRequestsPrefetch) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);
  auto prefetch_request = PrefetchTilesRequest();
  auto data_request = read::DataRequest()
                          .WithPartitionId(kTestPartition)
                          .WithFetchOption(FetchOptions::OnlineOnly);

  auto request_started = std::make_shared<std::promise<void>>();
  auto continue_request = std::make_shared<std::promise<void>>();

  {
    http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        request_started, continue_request,
        {http::HttpStatusCode::OK, HTTP_RESPONSE_BLOB_DATA_269});

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }

  auto data_future = client->GetData(data_request).GetFuture();
  auto prefetch_future = client->PrefetchTiles(prefetch_request).GetFuture();

  request_started->get_future().get();
  client->CancelPendingRequests();
  continue_request->set_value();

  ASSERT_EQ(prefetch_future.wait_for(kWaitTimeout), std::future_status::ready);
  auto prefetch_response = prefetch_future.get();

  ASSERT_FALSE(prefetch_response.IsSuccessful())
      << ApiErrorToString(prefetch_response.GetError());

  ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
            prefetch_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(client::ErrorCode::Cancelled,
            prefetch_response.GetError().GetErrorCode());

  ASSERT_EQ(data_future.wait_for(kWaitTimeout), std::future_status::ready);

  auto data_response = data_future.get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());

  ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       DISABLED_GetDataWithPartitionIdCancelLookupMetadata) {
  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::DataRequest();
  request.WithPartitionId(kTestPartition);

  std::promise<DataResponse> promise;
  auto callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  client::CancellationToken cancel_token = client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       DISABLED_GetDataWithPartitionIdCancelLatestCatalogVersion) {
  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LATEST_CATALOG_VERSION});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(0);

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::DataRequest();
  request.WithPartitionId(kTestPartition);

  std::promise<DataResponse> promise;
  auto callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  client::CancellationToken cancel_token = client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithPartitionIdCancelLookupQuery) {
  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
      .Times(0);

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::DataRequest();
  request.WithPartitionId(kTestPartition);

  std::promise<DataResponse> promise;
  auto callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  client::CancellationToken cancel_token = client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithPartitionIdCancelQuery) {
  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_PARTITION_269});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::DataRequest();
  request.WithPartitionId(kTestPartition);

  std::promise<DataResponse> promise;
  auto callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  client::CancellationToken cancel_token = client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithPartitionIdCancelLookupBlob) {
  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(0);

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::DataRequest();
  request.WithPartitionId(kTestPartition);

  std::promise<DataResponse> promise;
  auto callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  client::CancellationToken cancel_token = client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithPartitionIdCancelBlob) {
  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_BLOB_DATA_269});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::DataRequest();
  request.WithPartitionId(kTestPartition);

  std::promise<DataResponse> promise;
  auto callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  client::CancellationToken cancel_token = client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithPartitionIdVersion2) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 2, settings_);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LAYER_VERSIONS_V2), _, _, _, _))
      .Times(0);

  auto request = read::DataRequest();
  request.WithPartitionId(kTestPartition);
  auto data_response = client->GetData(request).GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031_V2", data_string);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithPartitionIdInvalidVersion) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 10, settings_);

  auto request = read::DataRequest();
  request.WithPartitionId(kTestPartition);
  auto data_response = client->GetData(request).GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(client::ErrorCode::BadRequest,
            data_response.GetError().GetErrorCode());
  ASSERT_EQ(http::HttpStatusCode::BAD_REQUEST,
            data_response.GetError().GetHttpStatusCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataCacheOnly) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(0);
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::DataRequest();
  request.WithPartitionId(kTestPartition)
      .WithFetchOption(FetchOptions::CacheOnly);
  auto future = client->GetData(request);
  auto data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataOnlineOnly) {
  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(4)
        .WillRepeatedly(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));
  }

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, boost::none, settings_);

  auto request = read::DataRequest();
  request.WithPartitionId(kTestPartition)
      .WithFetchOption(FetchOptions::OnlineOnly);
  auto future = client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031", data_string);
  // Should fail despite cached response
  future = client->GetData(request);
  data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetTile) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 4, settings_);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kHttpQueryTreeIndex_23064), _, _, _, _))
      .WillOnce(
          ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK, 1024, 123),
                             kHttpSubQuads_23064));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kHttpResponseBlobData_5904591), _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          GetResponse(http::HttpStatusCode::OK, 1024, 123), "someData"));

  auto request =
      read::TileRequest().WithTileKey(geo::TileKey::FromHereTile("5904591"));
  auto data_response = client->GetData(request).GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("someData", data_string);

  const auto& network_stats = data_response.GetPayload();
  EXPECT_EQ(network_stats.GetBytesDownloaded(), 2048);
  EXPECT_EQ(network_stats.GetBytesUploaded(), 246);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetTileConcurrent) {
  const std::chrono::milliseconds request_delay(100);

  // Setup the network mock to expect necessary call exact once
  {
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP, {}, request_delay));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kHttpQueryTreeIndex_23064), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kHttpSubQuads_23064, {}, request_delay));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kHttpResponseBlobData_5904591), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     "someData", {}, request_delay));
  }

  const auto concurrent_calls = 5;

  // Replace the default task_scheduler, since it uses only 1 thread
  settings_.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(
          concurrent_calls);

  read::VersionedLayerClient client(kCatalog, kTestLayer, 4, settings_);

  std::vector<std::promise<read::DataResponse>> promises(concurrent_calls);

  for (auto& promise : promises) {
    client.GetData(
        read::TileRequest().WithTileKey(geo::TileKey::FromHereTile("5904591")),
        placeholder(promise));
  }

  for (auto& promise : promises) {
    auto data_response = promise.get_future().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
    std::string data_string(data_response.GetResult()->begin(),
                            data_response.GetResult()->end());
    ASSERT_EQ("someData", data_string);
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetTileCacheOnly) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(0);
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 4, settings_);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kHttpQueryTreeIndex_23064), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kHttpResponseBlobData_5904591), _, _, _, _))
      .Times(0);

  auto request = read::TileRequest()
                     .WithTileKey(geo::TileKey::FromHereTile("5904591"))
                     .WithFetchOption(FetchOptions::CacheOnly);
  auto future = client->GetData(request);
  auto data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetTileOnlineOnly) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 4, settings_);
  {
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kHttpQueryTreeIndex_23064), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kHttpSubQuads_23064));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kHttpResponseBlobData_5904591), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     "someData"));
  }
  auto request =
      read::TileRequest().WithTileKey(geo::TileKey::FromHereTile("5904591"));
  {
    SCOPED_TRACE("Request data using TileKey.");
    auto data_response = client->GetData(request).GetFuture().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
    std::string data_string(data_response.GetResult()->begin(),
                            data_response.GetResult()->end());
    ASSERT_EQ("someData", data_string);
  }

  {
    SCOPED_TRACE("Check cached data.");
    auto future = client->GetData(request);
    auto data_response = future.GetFuture().get();
    ASSERT_TRUE(data_response.IsSuccessful());
  }
  {
    SCOPED_TRACE("Check OnlineOnly request");
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(4)
        .WillRepeatedly(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));
    auto future =
        client->GetData(request.WithFetchOption(FetchOptions::OnlineOnly));
    auto data_response = future.GetFuture().get();
    ASSERT_FALSE(data_response.IsSuccessful());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetTileCacheWithUpdate) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 4, settings_);

  auto request = read::TileRequest()
                     .WithTileKey(geo::TileKey::FromHereTile("5904591"))
                     .WithFetchOption(FetchOptions::CacheWithUpdate);
  {
    std::cout << "request<=" << request.GetFetchOption() << std::endl;
    SCOPED_TRACE("Request data using TileKey.");
    auto data_response = client->GetData(request).GetFuture().get();

    ASSERT_FALSE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
  }

  {
    SCOPED_TRACE("Check cache, should not be avialible.");
    auto future =
        client->GetData(request.WithFetchOption(FetchOptions::CacheOnly));
    auto data_response = future.GetFuture().get();
    ASSERT_FALSE(data_response.IsSuccessful());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetTileTwoSequentialCalls) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 4, settings_);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kHttpQueryTreeIndex_23064), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpSubQuads_23064));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kHttpResponseBlobData_5904591), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   "someData"));
  {
    SCOPED_TRACE("Request data using TileRequest");
    auto request =
        read::TileRequest().WithTileKey(geo::TileKey::FromHereTile("5904591"));
    auto data_response = client->GetData(request).GetFuture().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
    std::string data_string(data_response.GetResult()->begin(),
                            data_response.GetResult()->end());
    ASSERT_EQ("someData", data_string);
  }
  {
    SCOPED_TRACE("Neighboring tile, same subquad, check metadata cached");
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(0);

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kHttpQueryTreeIndex_23064), _, _, _, _))
        .Times(0);

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(0);

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kUrlBlobData_1476147), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     "someOtherData"));

    auto request =
        read::TileRequest().WithTileKey(geo::TileKey::FromHereTile("1476147"));
    auto future = client->GetData(request);
    auto data_response = future.GetFuture().get();
    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
    std::string data_string(data_response.GetResult()->begin(),
                            data_response.GetResult()->end());
    ASSERT_EQ("someOtherData", data_string);
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, RemoveFromCachePartition) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, kTestVersion, settings_);

  // load and cache some data
  auto promise = std::make_shared<std::promise<DataResponse>>();
  auto future = promise->get_future();

  auto data_request = read::DataRequest().WithPartitionId(kTestPartition);
  auto token = client->GetData(data_request, [promise](DataResponse response) {
    promise->set_value(response);
  });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_NE(response.GetResult(), nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);

  // remove the data from cache
  ASSERT_TRUE(client->RemoveFromCache(kTestPartition));

  // check the data is not available in cache
  promise = std::make_shared<std::promise<DataResponse>>();
  future = promise->get_future();
  data_request.WithFetchOption(FetchOptions::CacheOnly);
  token = client->GetData(data_request, [promise](DataResponse response) {
    promise->set_value(response);
  });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  response = future.get();

  ASSERT_FALSE(response.IsSuccessful());
}

TEST_F(DataserviceReadVersionedLayerClientTest, RemoveFromCacheTileKey) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_QUADKEYS_92259))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, kTestVersion, settings_);

  // load and cache some data
  auto promise = std::make_shared<std::promise<DataResponse>>();
  auto future = promise->get_future();
  auto tile_key = geo::TileKey::FromHereTile("23618364");
  auto data_request = read::TileRequest().WithTileKey(tile_key);
  auto token = client->GetData(data_request, [promise](DataResponse response) {
    promise->set_value(response);
  });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_NE(response.GetResult(), nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);

  // remove the data from cache
  ASSERT_TRUE(client->RemoveFromCache(tile_key));

  // check the data is not available in cache
  promise = std::make_shared<std::promise<DataResponse>>();
  future = promise->get_future();
  data_request.WithFetchOption(FetchOptions::CacheOnly);
  token = client->GetData(data_request, [promise](DataResponse response) {
    promise->set_value(response);
  });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  response = future.get();

  ASSERT_FALSE(response.IsSuccessful());
}

TEST_F(DataserviceReadVersionedLayerClientTest, CheckIfPartitionCached) {
  settings_.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});
  {
    SCOPED_TRACE("Download and check");

    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kHttpResponsePartition_269))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kHttpResponseBlobData_269));

    read::VersionedLayerClient client(kCatalog, kTestLayer, kTestVersion,
                                      settings_);

    auto data_request = read::DataRequest().WithPartitionId(kTestPartition);
    auto future = client.GetData(data_request).GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    DataResponse response = future.get();

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_NE(response.GetResult(), nullptr);
    ASSERT_NE(response.GetResult()->size(), 0u);

    // check the data is available in cache
    ASSERT_TRUE(client.IsCached(kTestPartition));
  }
  {
    SCOPED_TRACE("Client without version can't check");
    read::VersionedLayerClient client(kCatalog, kTestLayer, boost::none,
                                      settings_);
    EXPECT_FALSE(client.IsCached(kTestPartition));
  }
  {
    SCOPED_TRACE("IsCached after removal");
    read::VersionedLayerClient client(kCatalog, kTestLayer, kTestVersion,
                                      settings_);
    ASSERT_TRUE(client.IsCached(kTestPartition));
    ASSERT_TRUE(client.RemoveFromCache(kTestPartition));
    ASSERT_FALSE(client.IsCached(kTestPartition));
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, CheckIfTileKeyCached) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_QUADKEYS_92259))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, kTestVersion, settings_);

  // load and cache some data
  auto promise = std::make_shared<std::promise<DataResponse>>();
  auto future = promise->get_future();
  auto tile_key = geo::TileKey::FromHereTile("23618364");
  auto data_request = read::TileRequest().WithTileKey(tile_key);
  auto token = client->GetData(data_request, [promise](DataResponse response) {
    promise->set_value(response);
  });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_NE(response.GetResult(), nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);

  // check the data is available in cache
  ASSERT_TRUE(client->IsCached(tile_key));

  // remove the data from cache
  ASSERT_TRUE(client->RemoveFromCache(tile_key));

  ASSERT_FALSE(client->IsCached(tile_key));
}

TEST_F(DataserviceReadVersionedLayerClientTest, CheckLookupApiCacheExpiration) {
  using testing::Return;

  // initialize mock cache
  auto cache = std::make_shared<testing::StrictMock<CacheMock>>();
  settings_.cache = cache;

  auto client = read::VersionedLayerClient(kCatalog, kTestLayer, 4, settings_);

  // check if expiration time is 1 hour(3600 sec)
  time_t expiration_time = 3600;
  EXPECT_CALL(*cache, Get("hrn:here:data::olp-here-test:hereos-internal-test-"
                          "v2::query::v1::api",
                          _))
      .Times(1)
      .WillOnce(Return(boost::any()));
  EXPECT_CALL(*cache, Put("hrn:here:data::olp-here-test:hereos-internal-test-"
                          "v2::query::v1::api",
                          _, _, expiration_time))
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(*cache, Get("hrn:here:data::olp-here-test:hereos-internal-test-"
                          "v2::blob::v1::api",
                          _))
      .Times(1)
      .WillOnce(
          Return(std::string("https://blob-ireland.data.api.platform.here.com/"
                             "blobstore/v1/catalogs/hereos-internal-test-v2")));

  EXPECT_CALL(*cache, Put("hrn:here:data::olp-here-test:hereos-internal-test-"
                          "v2::blob::v1::api",
                          _, _, expiration_time))
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(*cache, Put("hrn:here:data::olp-here-test:hereos-internal-test-"
                          "v2::metadata::v1::api",
                          _, _, expiration_time))
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(*cache, Put("hrn:here:data::olp-here-test:hereos-internal-test-"
                          "v2::volatile-blob::v1::api",
                          _, _, expiration_time))
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(*cache, Put("hrn:here:data::olp-here-test:hereos-internal-test-"
                          "v2::stream::v2::api",
                          _, _, expiration_time))
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(*cache, Read("hrn:here:data::olp-here-test:hereos-internal-test-"
                           "v2::testlayer::269::4::partition"))
      .Times(1)
      .WillOnce(Return(client::ApiError::NotFound()));
  EXPECT_CALL(*cache, Write("hrn:here:data::olp-here-test:hereos-internal-test-"
                            "v2::testlayer::269::4::partition",
                            _, _))
      .Times(1)
      .WillOnce(Return(client::ApiNoResult{}));

  EXPECT_CALL(*cache,
              Get("hrn:here:data::olp-here-test:hereos-internal-test-v2:"
                  ":testlayer::4eed6ed1-0d32-43b9-ae79-043cb4256432::Data"))
      .Times(1)
      .WillOnce(Return(nullptr));

  EXPECT_CALL(*cache,
              Write("hrn:here:data::olp-here-test:hereos-internal-test-v2:"
                    ":testlayer::4eed6ed1-0d32-43b9-ae79-043cb4256432::Data",
                    _, _))
      .Times(1)
      .WillOnce(Return(client::ApiNoResult{}));

  auto request = read::DataRequest()
                     .WithPartitionId(kTestPartition)
                     .WithFetchOption(FetchOptions::OnlineIfNotFound);
  auto future = client.GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031", data_string);
}

TEST_F(DataserviceReadVersionedLayerClientTest, Eviction) {
  constexpr auto data_handle_prefix = "e119d20e-";
  const auto data_size = 64u * 1024u;
  const std::string blob_data(data_size, 0);
  const std::string cache_path =
      olp::utils::Dir::TempDirectory() + "/integration_test";

  olp::cache::CacheSettings cache_settings;
  cache_settings.disk_path_mutable = cache_path;
  cache_settings.max_memory_cache_size = 0u;
  cache_settings.eviction_policy =
      olp::cache::EvictionPolicy::kLeastRecentlyUsed;
  cache_settings.max_disk_storage = 10u * 1024u * 1024u;
  settings_.cache =
      client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);
  settings_.cache->RemoveKeysWithPrefix({});

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, kTestVersion, settings_);

  auto create_partition_response = [](const std::string& partition,
                                      const std::string& data_handle) {
    constexpr auto begin =
        R"jsonString({ "partitions": [{"version":4,"partition":")jsonString";
    constexpr auto middle =
        R"jsonString(","layer":"testlayer","dataHandle":")jsonString";
    constexpr auto end = R"jsonString("}]})jsonString";
    return begin + partition + middle + data_handle + end;
  };

  auto is_in_cache = [&client](const std::string& key) {
    const auto request =
        read::DataRequest().WithPartitionId(key).WithFetchOption(
            FetchOptions::CacheOnly);
    auto future = client->GetData(request).GetFuture();
    return future.get().IsSuccessful();
  };

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP));

  const auto promote_key = std::to_string(0);
  const auto evicted_key = std::to_string(1);

  // overflow the mutable cache
  auto count = 0u;
  auto partition = std::to_string(count);
  const auto max_count = cache_settings.max_disk_storage / data_size;
  for (; count < max_count; ++count) {
    partition = std::to_string(count);
    const auto data_handle = data_handle_prefix + partition;
    const auto partition_response =
        create_partition_response(partition, data_handle);

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequestPrefix(kUrlPartitionsPrefix), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     partition_response));
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequestPrefix(kUrlBlobstorePrefix), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     blob_data));

    const auto request = read::DataRequest().WithPartitionId(partition);
    auto future = client->GetData(request).GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    ASSERT_TRUE(future.get().IsSuccessful());

    // promote the key so its not evicted.
    ASSERT_TRUE(is_in_cache(promote_key));
  }

  // maximum is reached.
  ASSERT_TRUE(count == max_count);
  EXPECT_TRUE(is_in_cache(promote_key));
  EXPECT_TRUE(is_in_cache(partition));
  EXPECT_FALSE(is_in_cache(evicted_key));
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetAggregatedData) {
  const auto tile_key = geo::TileKey::FromHereTile("23618364");

  // Disable cache as we rewrite same quad tree in each test case.
  settings_.cache.reset();

  {
    SCOPED_TRACE("Same tile");
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP));
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_AGGREGATE_92259), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK, 1024, 123),
                               HTTP_RESPONSE_QUADKEYS_92259));
    EXPECT_CALL(
        *network_mock_,
        Send(IsGetRequest(URL_BLOB_AGGREGATE_DATA_23618364), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::OK, 1024, 123), "some_data"));

    auto client = read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion,
                                             settings_);

    auto future =
        client.GetAggregatedData(read::TileRequest().WithTileKey(tile_key))
            .GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    const auto response = future.get();
    const auto& result = response.GetResult();

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_TRUE(result.GetData());
    ASSERT_EQ(result.GetTile(), tile_key);

    const auto& network_stats = response.GetPayload();
    EXPECT_EQ(network_stats.GetBytesDownloaded(), 2048);
    EXPECT_EQ(network_stats.GetBytesUploaded(), 246);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }

  {
    SCOPED_TRACE("Root ancestor tile");
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP));
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_AGGREGATE_92259), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_QUADKEYS_92259_ROOT_ONLY));
    EXPECT_CALL(
        *network_mock_,
        Send(IsGetRequest(URL_BLOB_AGGREGATE_DATA_23618364), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     "some_data"));

    auto client = read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion,
                                             settings_);
    const auto expected_tile = tile_key.ChangedLevelBy(-4);

    auto future =
        client.GetAggregatedData(read::TileRequest().WithTileKey(tile_key))
            .GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    const auto response = future.get();
    const auto& result = response.GetResult();

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_TRUE(result.GetData());
    ASSERT_EQ(result.GetTile(), expected_tile);
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }

  {
    SCOPED_TRACE("No ancestors");
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP));
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_AGGREGATE_92259), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                               HTTP_RESPONSE_QUADKEYS_92259_NO_ANCESTORS));

    auto client = read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion,
                                             settings_);

    auto future =
        client.GetAggregatedData(read::TileRequest().WithTileKey(tile_key))
            .GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    const auto response = future.get();
    const auto& error = response.GetError();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(error.GetErrorCode(), client::ErrorCode::NotFound);
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Empty quad tree");
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP));
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_AGGREGATE_92259), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK, 1024, 100),
                               HTTP_RESPONSE_NO_QUADS));

    auto client = read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion,
                                             settings_);

    auto future =
        client.GetAggregatedData(read::TileRequest().WithTileKey(tile_key))
            .GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    const auto response = future.get();
    const auto& error = response.GetError();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(error.GetErrorCode(), client::ErrorCode::NotFound);

    const auto& network_stats = response.GetPayload();
    EXPECT_EQ(network_stats.GetBytesDownloaded(), 1024);
    EXPECT_EQ(network_stats.GetBytesUploaded(), 100);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetAggregatedDataCacheQuadTree) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP));
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUADKEYS_AGGREGATE_92259), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_QUADKEYS_92259));
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_BLOB_AGGREGATE_DATA_23618364), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   "some_data"));

  const auto tile_key = geo::TileKey::FromHereTile("23618364");

  olp::cache::CacheSettings cache_settings;
  settings_.cache =
      client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);

  auto client =
      read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion, settings_);

  auto future =
      client.GetAggregatedData(read::TileRequest().WithTileKey(tile_key))
          .GetFuture();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  const auto response = future.get();
  const auto& result = response.GetResult();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_TRUE(result.GetData());
  ASSERT_EQ(result.GetTile(), tile_key);

  // try to load quadtree and the tile from cache
  future = client
               .GetAggregatedData(
                   read::TileRequest().WithTileKey(tile_key).WithFetchOption(
                       FetchOptions::CacheOnly))
               .GetFuture();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  const auto cache_response = future.get();
  const auto& cache_result = cache_response.GetResult();

  ASSERT_TRUE(cache_response.IsSuccessful())
      << cache_response.GetError().GetMessage();
  ASSERT_TRUE(result.GetData());
  ASSERT_EQ(cache_result.GetTile(), tile_key);
  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetAggregatedDataAsync) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP));
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUADKEYS_AGGREGATE_92259), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_QUADKEYS_92259));
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_BLOB_AGGREGATE_DATA_23618364), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   "some_data"));

  auto client =
      read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion, settings_);

  auto promise = std::make_shared<std::promise<AggregatedDataResponse>>();
  std::future<AggregatedDataResponse> future = promise->get_future();
  const auto tile_key = geo::TileKey::FromHereTile("23618364");
  auto token =
      client.GetAggregatedData(read::TileRequest().WithTileKey(tile_key),
                               [promise](AggregatedDataResponse response) {
                                 promise->set_value(response);
                               });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  auto response = future.get();
  const auto& result = response.GetResult();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_TRUE(result.GetData());
  ASSERT_EQ(result.GetTile(), tile_key);
  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetAggregatedDataAsyncConcurrent) {
  const auto delay = std::chrono::milliseconds(100);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP, {}, delay));
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUADKEYS_AGGREGATE_92259), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_QUADKEYS_92259, {}, delay));
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_BLOB_AGGREGATE_DATA_23618364), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   "some_data", {}, delay));
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_BLOB_AGGREGATE_DATA_23618363), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   "some_data", {}, delay));

  const auto concurrent_calls = 5;

  settings_.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(
          concurrent_calls);

  std::vector<std::promise<read::AggregatedDataResponse>> promises(
      concurrent_calls);

  read::VersionedLayerClient client(kCatalog, kTestLayer, kTestVersion,
                                    settings_);

  auto index = 0;

  const geo::TileKey tiles[] = {geo::TileKey::FromHereTile("23618364"),
                                geo::TileKey::FromHereTile("23618363")};

  // Request 2 neightbor tiles in parallel, make sure the tree and blobs are
  // downloaded only once.
  for (auto& promise : promises) {
    const auto tile = tiles[index++ % 2];
    client.GetAggregatedData(read::TileRequest().WithTileKey(tile),
                             placeholder(promise));
  }

  for (auto& promise : promises) {
    auto response = promise.get_future().get();
    const auto& result = response.GetResult();

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_TRUE(result.GetData());
  }

  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetAggregatedDataErrors) {
  const auto tile_key = geo::TileKey::FromHereTile("23618364");

  {
    SCOPED_TRACE("CacheWithUpdate");

    auto client = read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion,
                                             settings_);

    auto future =
        client
            .GetAggregatedData(
                read::TileRequest().WithTileKey(tile_key).WithFetchOption(
                    FetchOptions::CacheWithUpdate))
            .GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    const auto response = future.get();
    const auto& error = response.GetError();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(error.GetErrorCode(), client::ErrorCode::InvalidArgument);
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }

  {
    SCOPED_TRACE("Request without tile key");

    auto client = read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion,
                                             settings_);

    auto future = client.GetAggregatedData(read::TileRequest()).GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    const auto response = future.get();
    const auto& error = response.GetError();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(error.GetErrorCode(), client::ErrorCode::InvalidArgument);
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetAggregatedDataCancelableFuture) {
  const auto tile_key = geo::TileKey::FromHereTile("23618364");

  // Disable cache as we rewrite same quad tree in several test cases.
  settings_.cache.reset();

  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  {
    SCOPED_TRACE("Lookup API");

    auto wait_for_cancel = std::make_shared<std::promise<void>>();
    auto pause_for_cancel = std::make_shared<std::promise<void>>();

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        wait_for_cancel, pause_for_cancel,
        {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_mock_, Cancel(_))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    auto client = read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion,
                                             settings_);

    auto cancellable =
        client.GetAggregatedData(read::TileRequest().WithTileKey(tile_key));

    wait_for_cancel->get_future().get();
    cancellable.GetCancellationToken().Cancel();
    pause_for_cancel->set_value();

    auto future = cancellable.GetFuture();
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    const auto response = future.get();
    const auto& error = response.GetError();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(error.GetErrorCode(), client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }

  {
    SCOPED_TRACE("Quadkeys");

    auto wait_for_cancel = std::make_shared<std::promise<void>>();
    auto pause_for_cancel = std::make_shared<std::promise<void>>();

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        wait_for_cancel, pause_for_cancel,
        {http::HttpStatusCode::OK, HTTP_RESPONSE_QUADKEYS_92259});

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP));
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_AGGREGATE_92259), _, _, _, _))
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_mock_, Cancel(_))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    auto client = read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion,
                                             settings_);

    auto cancellable =
        client.GetAggregatedData(read::TileRequest().WithTileKey(tile_key));

    wait_for_cancel->get_future().get();
    cancellable.GetCancellationToken().Cancel();
    pause_for_cancel->set_value();

    auto future = cancellable.GetFuture();
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    const auto response = future.get();
    const auto& error = response.GetError();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(error.GetErrorCode(), client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }

  {
    SCOPED_TRACE("Blob data");

    auto wait_for_cancel = std::make_shared<std::promise<void>>();
    auto pause_for_cancel = std::make_shared<std::promise<void>>();

    std::tie(request_id, send_mock, cancel_mock) =
        GenerateNetworkMockActions(wait_for_cancel, pause_for_cancel,
                                   {http::HttpStatusCode::OK, "some_data"});

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP));
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_AGGREGATE_92259), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_QUADKEYS_92259));
    EXPECT_CALL(
        *network_mock_,
        Send(IsGetRequest(URL_BLOB_AGGREGATE_DATA_23618364), _, _, _, _))
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_mock_, Cancel(_))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    auto client = read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion,
                                             settings_);

    auto cancellable =
        client.GetAggregatedData(read::TileRequest().WithTileKey(tile_key));

    wait_for_cancel->get_future().get();
    cancellable.GetCancellationToken().Cancel();
    pause_for_cancel->set_value();

    auto future = cancellable.GetFuture();
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    const auto response = future.get();
    const auto& error = response.GetError();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(error.GetErrorCode(), client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetAggregatedDataCancel) {
  const auto tile_key = geo::TileKey::FromHereTile("23618364");
  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));
  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto client =
      read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion, settings_);

  std::promise<AggregatedDataResponse> promise;
  auto token =
      client.GetAggregatedData(read::TileRequest().WithTileKey(tile_key),
                               [&promise](AggregatedDataResponse response) {
                                 promise.set_value(response);
                               });

  wait_for_cancel->get_future().get();
  token.Cancel();
  pause_for_cancel->set_value();

  auto future = promise.get_future();
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  const auto response = future.get();
  const auto& error = response.GetError();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(error.GetErrorCode(), client::ErrorCode::Cancelled);
  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetAggregatedDataCancelOnClientDeletion) {
  const auto tile_key = geo::TileKey::FromHereTile("23618364");
  http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));
  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, kTestVersion, settings_);

  auto cancellable =
      client->GetAggregatedData(read::TileRequest().WithTileKey(tile_key));

  wait_for_cancel->get_future().get();
  client.reset();
  pause_for_cancel->set_value();

  auto future = cancellable.GetFuture();
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  const auto response = future.get();
  const auto& error = response.GetError();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(error.GetErrorCode(), client::ErrorCode::Cancelled);
  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetAggregatedDataPriority) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP));
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kHttpQueryTreeIndex_23064), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpSubQuads_23064));
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kHttpResponseBlobData_5904591), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   "someData"));

  constexpr auto version = 4;
  auto scheduler = settings_.task_scheduler;
  std::promise<void> block_promise;
  std::promise<void> finish_promise;
  auto block_future = block_promise.get_future();
  auto finish_future = finish_promise.get_future();

  scheduler->ScheduleTask([&]() { block_future.wait_for(kWaitTimeout); },
                          std::numeric_limits<uint32_t>::max());

  auto priority = 700u;
  // this priority should be less than `priority`, but greater than NORMAL
  auto finish_task_priority = 600u;

  auto client =
      read::VersionedLayerClient(kCatalog, kTestLayer, version, settings_);

  auto request = read::TileRequest()
                     .WithTileKey(geo::TileKey::FromHereTile("5904591"))
                     .WithPriority(priority);
  auto future = client.GetAggregatedData(request).GetFuture();
  scheduler->ScheduleTask(
      [&]() {
        EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)),
                  std::future_status::ready);
        finish_promise.set_value();
      },
      finish_task_priority);

  // unblock queue
  block_promise.set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  ASSERT_NE(finish_future.wait_for(kWaitTimeout), std::future_status::timeout);

  auto response = future.get();
  auto& data = response.GetResult().GetData();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_NE(data, nullptr);
  ASSERT_NE(data->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetTileAndAggregatedData) {
  olp::cache::CacheSettings cache_settings;
  auto client = read::VersionedLayerClient(kCatalog, kTestLayer, 4, settings_);

  {
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kHttpQueryTreeIndex_23064), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kHttpSubQuads_23064));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kHttpResponseBlobData_5904591), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     "someData"));
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kUrlBlobData_1476147), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     "someData"));

    auto tiles = {geo::TileKey::FromHereTile("5904591"),
                  geo::TileKey::FromHereTile("1476147")};
    for (const auto& tile : tiles) {
      auto request = read::TileRequest().WithTileKey(tile);
      auto data_response = client.GetData(request).GetFuture().get();

      ASSERT_TRUE(data_response.IsSuccessful())
          << ApiErrorToString(data_response.GetError());
      ASSERT_LT(0, data_response.GetResult()->size());
      std::string data_string(data_response.GetResult()->begin(),
                              data_response.GetResult()->end());
      ASSERT_EQ("someData", data_string);
    }
  }
  {
    const auto tile_key = geo::TileKey::FromHereTile("1476147");

    // try to load quadtree and get tile from cache
    auto future =
        client
            .GetAggregatedData(
                read::TileRequest().WithTileKey(tile_key).WithFetchOption(
                    FetchOptions::CacheOnly))
            .GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    const auto response = future.get();
    const auto& result = response.GetResult();

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_TRUE(result.GetData());
    ASSERT_EQ(result.GetTile(), tile_key);
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, PrefetchTilesAndGetAggregated) {
  constexpr auto kLayerId = "hype-test-prefetch";

  auto client =
      read::VersionedLayerClient(kCatalog, kLayerId, boost::none, settings_);

  {
    SCOPED_TRACE("Prefetch tiles online and store them in memory cache");
    std::vector<geo::TileKey> tile_keys = {
        geo::TileKey::FromHereTile("5904591")};

    auto request = read::PrefetchTilesRequest()
                       .WithTileKeys(tile_keys)
                       .WithMinLevel(11)
                       .WithMaxLevel(12);

    auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
    auto future = promise->get_future();
    auto token = client.PrefetchTiles(
        request, [promise](PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PrefetchTilesResponse response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();

    for (auto tile_result : result) {
      ASSERT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }
  {
    SCOPED_TRACE("Get aggregated tile");
    const auto tile_key = geo::TileKey::FromHereTile("5904591");

    // try to load quadtree and the tile from cache
    auto future =
        client
            .GetAggregatedData(
                read::TileRequest().WithTileKey(tile_key).WithFetchOption(
                    FetchOptions::CacheOnly))
            .GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    const auto cache_response = future.get();
    const auto& cache_result = cache_response.GetResult();

    ASSERT_TRUE(cache_response.IsSuccessful())
        << cache_response.GetError().GetMessage();
    ASSERT_EQ(cache_result.GetTile(), tile_key);
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, ProtectAndReleaseTileKeys) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_QUADKEYS_92259))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  olp::cache::CacheSettings cache_settings;
  const std::string cache_path =
      olp::utils::Dir::TempDirectory() + "/integration_test";
  olp::utils::Dir::remove(cache_path);
  cache_settings.disk_path_mutable = cache_path;
  std::shared_ptr<olp::cache::KeyValueCache> cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);

  auto settings = settings_;
  settings.cache = cache;
  settings.default_cache_expiration = std::chrono::seconds(2);

  cache->RemoveKeysWithPrefix({});
  auto client =
      read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion, settings);

  // load and cache some data
  auto promise = std::make_shared<std::promise<DataResponse>>();
  auto future = promise->get_future();
  auto tile_key = geo::TileKey::FromHereTile("23618364");
  auto data_request = read::TileRequest().WithTileKey(tile_key);
  auto token = client.GetData(data_request, [promise](DataResponse response) {
    promise->set_value(response);
  });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_NE(response.GetResult(), nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);

  // check the data is available in cache
  ASSERT_TRUE(client.IsCached(tile_key));

  // protect tile and check that it is not expire
  ASSERT_TRUE(client.Protect({tile_key}));
  std::this_thread::sleep_for(std::chrono::seconds(3));
  ASSERT_TRUE(client.IsCached(tile_key));

  // release and check that tile not longer cached
  ASSERT_TRUE(client.Release({tile_key}));
  ASSERT_FALSE(client.IsCached(tile_key));
  // remove cache
  olp::utils::Dir::remove(cache_path);
}

TEST_F(DataserviceReadVersionedLayerClientTest, ProtectAndReleasePartition) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  olp::cache::CacheSettings cache_settings;
  const std::string cache_path =
      olp::utils::Dir::TempDirectory() + "/integration_test";
  olp::utils::Dir::remove(cache_path);
  cache_settings.disk_path_mutable = cache_path;
  std::shared_ptr<olp::cache::KeyValueCache> cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);

  auto settings = settings_;
  settings.cache = cache;
  settings.default_cache_expiration = std::chrono::seconds(2);

  cache->RemoveKeysWithPrefix({});
  auto client =
      read::VersionedLayerClient(kCatalog, kTestLayer, kTestVersion, settings);

  auto promise = std::make_shared<std::promise<DataResponse>>();
  std::future<DataResponse> future = promise->get_future();

  auto token = client.GetData(
      read::DataRequest().WithPartitionId(kTestPartition),
      [promise](DataResponse response) { promise->set_value(response); });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_NE(response.GetResult(), nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);

  // protect tile and check that it is not expire
  ASSERT_TRUE(client.Protect(kTestPartition));
  std::this_thread::sleep_for(std::chrono::seconds(3));
  ASSERT_TRUE(client.IsCached(kTestPartition));
  ASSERT_TRUE(client.Release(kTestPartition));
  ASSERT_FALSE(client.IsCached(kTestPartition));
}

TEST_F(DataserviceReadVersionedLayerClientTest, ProtectAndReleaseWithEviction) {
  constexpr auto kLayerId = "hype-test-prefetch";
  const auto data_size = 1024u;
  const std::string blob_data(data_size, 0);
  const std::string cache_path =
      olp::utils::Dir::TempDirectory() + "/integration_test";
  olp::utils::Dir::remove(cache_path);
  olp::cache::CacheSettings cache_settings;
  cache_settings.disk_path_mutable = cache_path;
  cache_settings.max_memory_cache_size = 0u;
  cache_settings.eviction_policy =
      olp::cache::EvictionPolicy::kLeastRecentlyUsed;
  cache_settings.max_disk_storage = 3u * 1024u;
  settings_.cache =
      client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);

  auto client = read::VersionedLayerClient(kCatalog, kLayerId, 4, settings_);

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP));
  const auto protected_key = geo::TileKey::FromHereTile("23618364");
  const auto evicted_key = geo::TileKey::FromHereTile("23618365");
  const auto other_key = geo::TileKey::FromHereTile("23618366");

  {
    SCOPED_TRACE("Get protected_key and store in cache");
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP));
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_QUADKEYS_92259));
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_3), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     blob_data));

    const auto request = read::TileRequest().WithTileKey(protected_key);
    auto future = client.GetData(request).GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    ASSERT_TRUE(future.get().IsSuccessful());

    // protect key so its not evicted.
    ASSERT_TRUE(client.Protect({protected_key}));
  }
  {
    SCOPED_TRACE("Get evicted_key and store in cache");
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_2), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     blob_data));

    const auto request = read::TileRequest().WithTileKey(evicted_key);
    auto future = client.GetData(request).GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    ASSERT_TRUE(future.get().IsSuccessful());
  }
  {
    SCOPED_TRACE("Get other_key and store in cache");
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_1), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     blob_data));

    const auto request = read::TileRequest().WithTileKey(other_key);
    auto future = client.GetData(request).GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    ASSERT_TRUE(future.get().IsSuccessful());
  }

  // maximum is reached, evicted_key is not longer in cache
  ASSERT_TRUE(client.IsCached(protected_key));
  ASSERT_FALSE(client.IsCached(evicted_key));
  ASSERT_TRUE(client.IsCached(other_key));

  // now release key
  ASSERT_TRUE(client.Release({protected_key}));

  {
    SCOPED_TRACE("Promoute other_key");
    const auto request = read::TileRequest().WithTileKey(other_key);
    auto future = client.GetData(request).GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    ASSERT_TRUE(future.get().IsSuccessful());
    ASSERT_TRUE(client.IsCached(other_key));
  }
  {
    SCOPED_TRACE("Write evicted key again");
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_2), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     blob_data));

    const auto request = read::TileRequest().WithTileKey(evicted_key);
    auto future = client.GetData(request).GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    ASSERT_TRUE(future.get().IsSuccessful());
  }
  ASSERT_TRUE(client.IsCached(evicted_key));
  ASSERT_TRUE(client.IsCached(other_key));
  // after release key is evicted
  ASSERT_FALSE(client.IsCached(protected_key));
  // remove cache folder
  olp::utils::Dir::remove(cache_path);
}

TEST_F(DataserviceReadVersionedLayerClientTest, CatalogEndpointProvider) {
  // Disable cache as we don't want to save APIs to its.
  settings_.cache.reset();

  {
    SCOPED_TRACE("Static url catalog");

    // lookup request shouldn't happen
    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kHttpResponsePartition_269))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kHttpResponseBlobData_269));

    const std::string provider_url = "https://some-lookup-url.com/lookup/v1";
    client::ApiLookupSettings lookup_settings;
    lookup_settings.catalog_endpoint_provider =
        [&provider_url](const client::HRN&) { return provider_url; };
    settings_.api_lookup_settings = lookup_settings;

    auto client = std::make_shared<read::VersionedLayerClient>(
        kCatalog, kTestLayer, 0, settings_);
    auto future =
        client->GetData(read::DataRequest().WithPartitionId(kTestPartition))
            .GetFuture();

    auto response = future.get();

    ASSERT_TRUE(response.IsSuccessful());
    ASSERT_TRUE(response.GetResult() != nullptr);
    ASSERT_NE(response.GetResult()->size(), 0u);
  }

  {
    SCOPED_TRACE("Empty url");

    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kHttpResponsePartition_269))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kHttpResponseBlobData_269));

    client::ApiLookupSettings lookup_settings;
    lookup_settings.catalog_endpoint_provider = [](const client::HRN&) {
      return "";
    };
    settings_.api_lookup_settings = lookup_settings;

    auto client = std::make_shared<read::VersionedLayerClient>(
        kCatalog, kTestLayer, 0, settings_);
    auto future =
        client->GetData(read::DataRequest().WithPartitionId(kTestPartition))
            .GetFuture();

    auto response = future.get();

    ASSERT_TRUE(response.IsSuccessful());
    ASSERT_TRUE(response.GetResult() != nullptr);
    ASSERT_NE(response.GetResult()->size(), 0u);
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, OverlappingQuads) {
  const auto kLayerId = "hype-test-prefetch";
  const auto cache_quad_key_23606936 =
      "hrn:here:data::olp-here-test:hereos-internal-test-v2::hype-test-"
      "prefetch::23606936::4::4::quadtree";
  const auto cache_quad_key_5901734 =
      "hrn:here:data::olp-here-test:hereos-internal-test-v2::hype-test-"
      "prefetch::5901734::4::4::quadtree";
  const auto data_size = 100u;
  const std::string blob_data(data_size, 0);
  const std::string cache_path =
      olp::utils::Dir::TempDirectory() + "/integration_test";
  olp::utils::Dir::remove(cache_path);
  olp::cache::CacheSettings cache_settings;
  cache_settings.disk_path_mutable = cache_path;
  settings_.cache =
      client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);

  auto client = read::VersionedLayerClient(kCatalog, kLayerId, 4, settings_);

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP));
  const auto tile_key = geo::TileKey::FromHereTile("23606936");

  {
    SCOPED_TRACE("Prefetch tile");
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_LOOKUP));
    const auto quad_request =
        R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/versions/4/quadkeys/23606936/depths/4)";
    const auto quad_responce =
        R"jsonString({"subQuads":[{"subQuadKey":"1","version":0,"dataHandle":"23606936-data-handle","dataSize":100}],"parentQuads":[]})jsonString";
    const auto data_request =
        R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/data/23606936-data-handle)";

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(quad_request), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     quad_responce));

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(data_request), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     blob_data));

    const auto request = read::PrefetchTilesRequest()
                             .WithTileKeys({tile_key})
                             .WithMinLevel(12)
                             .WithMaxLevel(16);
    auto future = client.PrefetchTiles(request).GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    ASSERT_TRUE(future.get().IsSuccessful());
  }
  {
    SCOPED_TRACE("Protect tile");
    ASSERT_TRUE(client.Protect({tile_key}));
  }
  {
    SCOPED_TRACE("Prefetch another levels");
    auto quad_request =
        R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/versions/4/quadkeys/5901734/depths/4)";
    auto quad_responce =
        R"jsonString({"subQuads":[{"subQuadKey":"4","version":0,"dataHandle":"23606936-data-handle","dataSize":100}],"parentQuads":[]})jsonString";

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(quad_request), _, _, _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     quad_responce));

    const auto request = read::PrefetchTilesRequest()
                             .WithTileKeys({tile_key})
                             .WithMinLevel(11)
                             .WithMaxLevel(15);
    auto future = client.PrefetchTiles(request).GetFuture();

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    ASSERT_TRUE(future.get().IsSuccessful());
  }

  {
    SCOPED_TRACE("Release tiles, check if all quads released");
    ASSERT_TRUE(settings_.cache->Protect({cache_quad_key_5901734}));
    ASSERT_TRUE(settings_.cache->IsProtected(cache_quad_key_23606936));
    ASSERT_TRUE(settings_.cache->IsProtected(cache_quad_key_5901734));
    auto release_response = client.Release({tile_key});
    ASSERT_TRUE(release_response);
    ASSERT_FALSE(settings_.cache->IsProtected(cache_quad_key_23606936));
    ASSERT_FALSE(settings_.cache->IsProtected(cache_quad_key_5901734));
  }

  // remove cache folder
  olp::utils::Dir::remove(cache_path);
}

TEST_F(DataserviceReadVersionedLayerClientTest, QuadTreeIndex) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 4, settings_);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _));
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kHttpQueryTreeIndexWithAdditionalFields_23064),
                   _, _, _, _))
      .WillOnce(
          ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK, 1024, 100),
                             kHttpSubQuadsWithAdditionalFields_23064));

  std::promise<PartitionsResponse> promise;
  auto request =
      read::TileRequest().WithTileKey(geo::TileKey::FromHereTile("5904591"));
  auto cancellation_context =
      client->QuadTreeIndex(request, [&](PartitionsResponse response) {
        promise.set_value(std::move(response));
      });

  const auto response = promise.get_future().get();
  ASSERT_TRUE(response);

  const auto& network_stats = response.GetPayload();
  EXPECT_EQ(network_stats.GetBytesUploaded(), 100);
  EXPECT_EQ(network_stats.GetBytesDownloaded(), 1024);

  const auto& partitions = response.GetResult().GetPartitions();
  ASSERT_EQ(partitions.size(), 1u);

  const auto& partition = partitions.front();
  ASSERT_TRUE(partition.GetChecksum());
  EXPECT_EQ(*partition.GetChecksum(), "yyy");
  ASSERT_TRUE(partition.GetCrc());
  EXPECT_EQ(*partition.GetCrc(), "bbb");
  ASSERT_TRUE(partition.GetDataSize());
  EXPECT_EQ(*partition.GetDataSize(), 20);
}

TEST_F(DataserviceReadVersionedLayerClientTest, QuadTreeIndexFallback) {
  auto client = std::make_shared<read::VersionedLayerClient>(
      kCatalog, kTestLayer, 4, settings_);
  {
    SCOPED_TRACE("Load the data without additional fields to the cache");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _));
    EXPECT_CALL(
        *network_mock_,
        Send(IsGetRequest(kHttpQueryTreeIndexWithAdditionalFields_23064), _, _,
             _, _))
        .Times(2)
        .WillRepeatedly(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::OK), kHttpSubQuads_23064));

    std::promise<PartitionsResponse> promise;
    auto request =
        read::TileRequest().WithTileKey(geo::TileKey::FromHereTile("5904591"));
    auto cancellation_context =
        client->QuadTreeIndex(request, [&](PartitionsResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = promise.get_future().get();
    ASSERT_TRUE(response);

    const auto& partitions = response.GetResult().GetPartitions();
    ASSERT_EQ(partitions.size(), 1u);

    const auto& partition = partitions.front();
    EXPECT_FALSE(partition.GetChecksum());
    EXPECT_FALSE(partition.GetCrc());
    EXPECT_FALSE(partition.GetDataSize());
  }
  {
    SCOPED_TRACE("Load the data with CacheOnly fetch option");

    std::promise<PartitionsResponse> promise;
    auto request = read::TileRequest()
                       .WithTileKey(geo::TileKey::FromHereTile("5904591"))
                       .WithFetchOption(FetchOptions::CacheOnly);
    auto cancellation_context =
        client->QuadTreeIndex(request, [&](PartitionsResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = promise.get_future().get();
    ASSERT_TRUE(response);

    const auto& partitions = response.GetResult().GetPartitions();
    ASSERT_EQ(partitions.size(), 1u);

    const auto& partition = partitions.front();
    EXPECT_FALSE(partition.GetChecksum());
    EXPECT_FALSE(partition.GetCrc());
    EXPECT_FALSE(partition.GetDataSize());
  }
  {
    SCOPED_TRACE("Load the data when it is cached without additional fields");

    EXPECT_CALL(
        *network_mock_,
        Send(IsGetRequest(kHttpQueryTreeIndexWithAdditionalFields_23064), _, _,
             _, _))
        .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                     kHttpSubQuadsWithAdditionalFields_23064));

    std::promise<PartitionsResponse> promise;
    auto request =
        read::TileRequest().WithTileKey(geo::TileKey::FromHereTile("5904591"));
    auto cancellation_context =
        client->QuadTreeIndex(request, [&](PartitionsResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = promise.get_future().get();
    ASSERT_TRUE(response);

    const auto& partitions = response.GetResult().GetPartitions();
    ASSERT_EQ(partitions.size(), 1u);

    const auto& partition = partitions.front();
    ASSERT_TRUE(partition.GetChecksum());
    EXPECT_EQ(*partition.GetChecksum(), "yyy");
    ASSERT_TRUE(partition.GetCrc());
    EXPECT_EQ(*partition.GetCrc(), "bbb");
    ASSERT_TRUE(partition.GetDataSize());
    EXPECT_EQ(*partition.GetDataSize(), 20);
  }
  {
    SCOPED_TRACE("Load the data with CacheOnly fetch option again");

    std::promise<PartitionsResponse> promise;
    auto request = read::TileRequest()
                       .WithTileKey(geo::TileKey::FromHereTile("5904591"))
                       .WithFetchOption(FetchOptions::CacheOnly);
    auto cancellation_context =
        client->QuadTreeIndex(request, [&](PartitionsResponse response) {
          promise.set_value(std::move(response));
        });

    auto future = promise.get_future();
    ASSERT_EQ(future.wait_for(kWaitTimeout), std::future_status::ready);

    const auto response = future.get();
    ASSERT_TRUE(response);

    const auto& partitions = response.GetResult().GetPartitions();
    ASSERT_EQ(partitions.size(), 1u);

    const auto& partition = partitions.front();
    ASSERT_TRUE(partition.GetChecksum());
    EXPECT_EQ(*partition.GetChecksum(), "yyy");
    ASSERT_TRUE(partition.GetCrc());
    EXPECT_EQ(*partition.GetCrc(), "bbb");
    ASSERT_TRUE(partition.GetDataSize());
    EXPECT_EQ(*partition.GetDataSize(), 20);
  }
}

}  // namespace
