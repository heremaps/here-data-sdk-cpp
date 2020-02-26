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
#include <chrono>
#include <string>

#include <matchers/NetworkUrlMatchers.h>
#include <mocks/NetworkMock.h>
#include <olp/authentication/Settings.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/porting/make_unique.h>
#include <olp/dataservice/read/VersionedLayerClient.h>

#include "HttpResponses.h"

using namespace olp::dataservice::read;
using namespace olp::tests::common;
using namespace testing;

namespace {

std::string GetArgument(const std::string& name) {
  if (name == "dataservice_read_test_catalog") {
    return "hrn:here:data::olp-here-test:here-optimized-map-for-visualization-"
           "2";
  } else if (name == "dataservice_read_test_layer") {
    return "testlayer";
  } else if (name == "dataservice_read_test_partition") {
    return "269";
  } else if (name == "dataservice_read_test_layer_version") {
    return "108";
  }
  ADD_FAILURE() << "unknown argument!";
  return "";
}

std::string GetTestCatalog() {
  return GetArgument("dataservice_read_test_catalog");
}

std::string ApiErrorToString(const olp::client::ApiError& error) {
  std::ostringstream result_stream;
  result_stream << "ERROR: code: " << static_cast<int>(error.GetErrorCode())
                << ", status: " << error.GetHttpStatusCode()
                << ", message: " << error.GetMessage();
  return result_stream.str();
}

constexpr char kHttpResponseLookupQuery[] =
    R"jsonString([{"api":"query","version":"v1","baseURL":"https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";

constexpr char kHttpResponsePartition_269[] =
    R"jsonString({ "partitions": [{"version":4,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"}]})jsonString";

constexpr char kHttpResponsePartitionsEmpty[] =
    R"jsonString({ "partitions": []})jsonString";

constexpr char kHttpResponseLookupBlob[] =
    R"jsonString([{"api":"blob","version":"v1","baseURL":"https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";

constexpr char kHttpResponseBlobData_269[] = R"jsonString(DT_2_0031)jsonString";

constexpr char kHttpResponseLatestCatalogVersion[] =
    R"jsonString({"version":4})jsonString";

constexpr auto kWaitTimeout = std::chrono::seconds(3);

class DataserviceReadVersionedLayerClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    network_mock_ = std::make_shared<NetworkMock>();

    settings_ = std::make_shared<olp::client::OlpClientSettings>();
    settings_->network_request_handler = network_mock_;
    settings_->task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

    SetUpCommonNetworkMockCalls();
  }

  void TearDown() override {
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
    network_mock_.reset();
    settings_->task_scheduler.reset();
    settings_.reset();
  }

  void SetUpCommonNetworkMockCalls() {
    ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_LOOKUP_CONFIG));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_CONFIG));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_LOOKUP_METADATA));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_LATEST_CATALOG_VERSION));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_LAYER_VERSIONS), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_LAYER_VERSIONS));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_PARTITIONS));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_LOOKUP_QUERY));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_LOOKUP_BLOB));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_LAYER_VERSIONS_V2), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_LAYER_VERSIONS_V2));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS_V10), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_LAYER_VERSIONS_V2));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS_VN1), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_INVALID_VERSION_VN1));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_PARTITIONS_INVALID_LAYER), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_INVALID_LAYER));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS_V2), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_PARTITIONS_V2));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_LAYER_VERSIONS_V10), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_INVALID_VERSION_V10));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_LAYER_VERSIONS_VN1), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_INVALID_VERSION_VN1));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_QUADKEYS_1476147), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_QUADKEYS_1476147));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_QUADKEYS_5904591), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_QUADKEYS_5904591));
    ON_CALL(*network_mock_, Send(IsGetRequest(URL_QUADKEYS_1), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_QUADKEYS_5904591));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_1), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_BLOB_DATA_PREFETCH_1));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_2), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_BLOB_DATA_PREFETCH_2));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_4), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_BLOB_DATA_PREFETCH_4));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_5), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_BLOB_DATA_PREFETCH_5));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_6), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_BLOB_DATA_PREFETCH_6));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_BLOB_DATA_269));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_PARTITION_269));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_QUERY_PARTITION_269_V2), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_PARTITION_269_V2));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_269_V2), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_BLOB_DATA_269_V2));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_QUERY_PARTITION_269_V10), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_INVALID_VERSION_V10));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_QUERY_PARTITION_269_VN1), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_INVALID_VERSION_VN1));

    // Catch any non-interesting network calls that don't need to be verified
    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _)).Times(testing::AtLeast(0));
  }

 protected:
  std::shared_ptr<olp::client::OlpClientSettings> settings_;
  std::shared_ptr<NetworkMock> network_mock_;
};

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataFromPartitionAsync) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupQuery))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupBlob))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");
  auto version = std::stoi(GetArgument("dataservice_read_test_layer_version"));

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, version, *settings_);
  ASSERT_TRUE(client);

  auto promise = std::make_shared<std::promise<DataResponse>>();
  std::future<DataResponse> future = promise->get_future();
  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = client->GetData(
      olp::dataservice::read::DataRequest()
          .WithPartitionId(partition),
      [promise](DataResponse response) { promise->set_value(response); });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_NE(response.GetResult(), nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionAsyncWithCancellableFuture) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupQuery))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupBlob))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");
  auto version = std::stoi(GetArgument("dataservice_read_test_layer_version"));
  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, version, *settings_);

  auto partition = GetArgument("dataservice_read_test_partition");
  auto data_request = olp::dataservice::read::DataRequest()
                          .WithPartitionId(partition);
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
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupQuery))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupBlob))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");
  auto version = 0;

  auto sync_settings = *settings_;
  sync_settings.task_scheduler.reset();
  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, version, sync_settings);
  ASSERT_TRUE(client);

  DataResponse response;

  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = client->GetData(
      olp::dataservice::read::DataRequest()
          .WithPartitionId(partition),
      [&response](DataResponse resp) { response = std::move(resp); });
  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionSyncWithCancellableFuture) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupQuery))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupBlob))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");
  auto version = 0;

  auto sync_settings = *settings_;
  sync_settings.task_scheduler.reset();
  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, version, sync_settings);
  ASSERT_TRUE(client);

  auto partition = GetArgument("dataservice_read_test_partition");
  auto data_request = olp::dataservice::read::DataRequest()
                          .WithPartitionId(partition);
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
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupQuery))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLatestCatalogVersion))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupQuery))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupBlob))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto sync_settings = *settings_;
  sync_settings.task_scheduler.reset();
  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, sync_settings);
  ASSERT_TRUE(client);

  DataResponse response;

  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(boost::none)
          .WithPartitionId(partition),
      [&response](DataResponse resp) { response = std::move(resp); });
  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionSyncLatestVersionInvalid) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupQuery))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::FORBIDDEN),
                                   kHttpResponseLatestCatalogVersion));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto sync_settings = *settings_;
  sync_settings.task_scheduler.reset();
  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, sync_settings);
  ASSERT_TRUE(client);

  DataResponse response;

  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(boost::none)
          .WithPartitionId(partition),
      [&response](DataResponse resp) { response = std::move(resp); });
  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_FALSE(response.GetResult() != nullptr);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionCacheAndUpdateSync) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupQuery))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupBlob))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseBlobData_269));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");
  auto version = 0;

  auto sync_settings = *settings_;
  sync_settings.task_scheduler.reset();
  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, 269, sync_settings);
  ASSERT_TRUE(client);

  DataResponse response;

  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = client->GetData(
      olp::dataservice::read::DataRequest()
          .WithPartitionId(partition)
          .WithFetchOption(FetchOptions::CacheWithUpdate),
      [&response](DataResponse resp) { response = std::move(resp); });
  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_FALSE(response.GetResult() != nullptr);

  token = client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition)
          .WithFetchOption(FetchOptions::CacheOnly),
      [&response](DataResponse resp) { response = std::move(resp); });
  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataEmptyPartitionsSync) {
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupQuery))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponsePartitionsEmpty));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");
  auto version = 0;

  auto sync_settings = *settings_;
  sync_settings.task_scheduler.reset();
  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, version, sync_settings);
  ASSERT_TRUE(client);

  DataResponse response;

  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = client->GetData(
      olp::dataservice::read::DataRequest()
          .WithPartitionId(partition),
      [&response](DataResponse resp) { response = std::move(resp); });
  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_FALSE(response.GetResult() != nullptr);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionCancelLookup) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, kHttpResponseLookupQuery});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");
  auto version = std::stoi(GetArgument("dataservice_read_test_layer_version"));

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);
  ASSERT_TRUE(client);

  std::promise<DataResponse> promise;
  std::future<DataResponse> future = promise.get_future();
  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition),
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

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, kHttpResponseLookupQuery});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");
  auto version = std::stoi(GetArgument("dataservice_read_test_layer_version"));

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);
  ASSERT_TRUE(client);

  auto partition = GetArgument("dataservice_read_test_partition");
  auto data_request = olp::dataservice::read::DataRequest()
                          .WithVersion(version)
                          .WithPartitionId(partition);

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

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, kHttpResponsePartition_269});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupQuery))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");
  auto version = std::stoi(GetArgument("dataservice_read_test_layer_version"));

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);
  ASSERT_TRUE(client);

  std::promise<DataResponse> promise;
  std::future<DataResponse> future = promise.get_future();
  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition),
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

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, kHttpResponseLookupBlob});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupQuery))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");
  auto version = std::stoi(GetArgument("dataservice_read_test_layer_version"));

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);
  ASSERT_TRUE(client);

  std::promise<DataResponse> promise;
  std::future<DataResponse> future = promise.get_future();
  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition),
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

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, kHttpResponseBlobData_269});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupQuery))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponsePartition_269))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpResponseLookupBlob))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");
  auto version = std::stoi(GetArgument("dataservice_read_test_layer_version"));

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);
  ASSERT_TRUE(client);

  std::promise<DataResponse> promise;
  std::future<DataResponse> future = promise.get_future();
  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition)
          .WithFetchOption(FetchOptions::CacheWithUpdate),
      [&promise](DataResponse response) { promise.set_value(response); });

  wait_for_cancel->get_future().get();
  token.Cancel();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  DataResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_TRUE(response.GetResult() == nullptr);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsNoError) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
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

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetPartitionsCancellableFutureNoError) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto cancellable_future = client->GetPartitions(request);
  auto future = cancellable_future.GetFuture();
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(4u, response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetPartitionsCancellableFutureCancellation) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  settings_->task_scheduler->ScheduleTask(
      []() { std::this_thread::sleep_for(std::chrono::seconds(1)); });

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto cancellable_future = client->GetPartitions(request);
  auto future = cancellable_future.GetFuture();

  cancellable_future.GetCancellationToken().Cancel();

  ASSERT_EQ(std::future_status::ready, future.wait_for(kWaitTimeout));

  auto response = future.get();
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetEmptyPartitions) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_EMPTY_PARTITIONS));

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
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
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .Times(2)
        .WillRepeatedly(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(
                olp::http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .Times(1);
  }

  olp::client::RetrySettings retry_settings;
  retry_settings.retry_condition =
      [](const olp::client::HttpResponse& response) {
        return olp::http::HttpStatusCode::TOO_MANY_REQUESTS == response.status;
      };
  settings_->retry_settings = retry_settings;
  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
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
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .Times(2)
        .WillRepeatedly(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(
                olp::http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .Times(1);
  }

  olp::client::RetrySettings retry_settings;
  retry_settings.retry_condition =
      [](const olp::client::HttpResponse& response) {
        return olp::http::HttpStatusCode::TOO_MANY_REQUESTS == response.status;
      };
  settings_->retry_settings = retry_settings;
  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
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
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = "somewhat_not_okay";

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
  auto future = promise->get_future();
  auto token = client->GetPartitions(
      request,
      [promise](PartitionsResponse response) { promise->set_value(response); });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsCacheWithUpdate) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto wait_to_start_signal = std::make_shared<std::promise<void>>();
  auto pre_callback_wait = std::make_shared<std::promise<void>>();
  pre_callback_wait->set_value();
  auto wait_for_end_signal = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_to_start_signal, pre_callback_wait,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_PARTITIONS},
      wait_for_end_signal);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  // Request 1
  {
    auto promise = std::make_shared<std::promise<PartitionsResponse>>();
    auto future = promise->get_future();
    auto request = PartitionsRequest().WithFetchOption(CacheWithUpdate);
    auto token =
        client->GetPartitions(request, [promise](PartitionsResponse response) {
          promise->set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PartitionsResponse response = future.get();
    // Request 1 return. Cached value (nothing)
    ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  }

  wait_for_end_signal->get_future().get();

  // Request 2
  {
    auto promise = std::make_shared<std::promise<PartitionsResponse>>();
    auto future = promise->get_future();
    auto request = PartitionsRequest().WithFetchOption(CacheOnly);
    auto token =
        client->GetPartitions(request, [promise](PartitionsResponse response) {
          promise->set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PartitionsResponse response = future.get();
    // Cache should be available here.
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitions403CacheClear) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);
  {
    testing::InSequence s;
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::FORBIDDEN),
                                     HTTP_RESPONSE_403));
  }

  // Populate cache
  auto request = olp::dataservice::read::PartitionsRequest();

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
    request.WithFetchOption(OnlineOnly);

    auto promise = std::make_shared<std::promise<PartitionsResponse>>();
    auto future = promise->get_future();
    auto token =
        client->GetPartitions(request, [promise](PartitionsResponse response) {
          promise->set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PartitionsResponse response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(403, response.GetError().GetHttpStatusCode());
  }

  // Check for cached response
  {
    request.WithFetchOption(CacheOnly);
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
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   R"jsonString(kd3sdf\)jsonString"));

  auto request = olp::dataservice::read::PartitionsRequest();
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
  auto future = promise->get_future();
  auto token = client->GetPartitions(
      request,
      [promise](PartitionsResponse response) { promise->set_value(response); });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  PartitionsResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::ServiceUnavailable,
            response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetPartitionsCancelLookupMetadata) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_METADATA});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  auto request = olp::dataservice::read::PartitionsRequest();
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
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            response.GetError().GetHttpStatusCode())
      << response.GetError().GetMessage();
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode())
      << response.GetError().GetMessage();
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetPartitionsCancelLatestCatalogVersion) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LATEST_CATALOG_VERSION});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LAYER_VERSIONS), _, _, _, _))
      .Times(0);

  auto request = olp::dataservice::read::PartitionsRequest();
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
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            response.GetError().GetHttpStatusCode())
      << response.GetError().GetMessage();
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode())
      << response.GetError().GetMessage();
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetPartitionsCancelLayerVersions) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LAYER_VERSIONS});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(0);

  auto request = olp::dataservice::read::PartitionsRequest();
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
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            response.GetError().GetHttpStatusCode())
      << response.GetError().GetMessage();
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode())
      << response.GetError().GetMessage();
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsVersion2) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, 2, *settings_);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS_V2), _, _, _, _))
      .Times(1);

  auto request = olp::dataservice::read::PartitionsRequest();
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
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, 10, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
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
    ASSERT_EQ(olp::client::ErrorCode::BadRequest,
              response.GetError().GetErrorCode());
    ASSERT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
  }

  {
    request.WithVersion(-1);
    auto promise = std::make_shared<std::promise<PartitionsResponse>>();
    auto future = promise->get_future();
    auto token =
        client->GetPartitions(request, [promise](PartitionsResponse response) {
          promise->set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    PartitionsResponse response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(olp::client::ErrorCode::BadRequest,
              response.GetError().GetErrorCode());
    ASSERT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsCacheOnly) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(0);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithFetchOption(CacheOnly);
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
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, layer, *settings_);

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(
                olp::http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));
  }

  auto request = olp::dataservice::read::PartitionsRequest();
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
    request.WithFetchOption(OnlineOnly);
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
  olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "hype-test-prefetch";

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, kLayerId, *settings_);
  ASSERT_TRUE(client);

  {
    SCOPED_TRACE("Prefetch tiles online and store them in memory cache");
    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile("5904591")};

    auto request = olp::dataservice::read::PrefetchTilesRequest()
                       .WithTileKeys(tile_keys)
                       .WithMinLevel(10)
                       .WithMaxLevel(12);

    auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
    std::future<PrefetchTilesResponse> future = promise->get_future();
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

  {
    SCOPED_TRACE("Read cached data from pre-fetched sub-partition #1");
    auto promise = std::make_shared<std::promise<DataResponse>>();
    std::future<DataResponse> future = promise->get_future();
    auto token = client->GetData(olp::dataservice::read::DataRequest()
                                     .WithPartitionId("23618365")
                                     .WithFetchOption(CacheOnly),
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

    auto token = client->GetData(olp::dataservice::read::DataRequest()
                                     .WithPartitionId("1476147")
                                     .WithFetchOption(CacheOnly),
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
       PrefetchTilesWithCancellableFutureWrongLevels) {
  olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "hype-test-prefetch";

  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile("5904591")};

  auto request = olp::dataservice::read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(0)
                     .WithMaxLevel(0);

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, kLayerId, *settings_);
  ASSERT_TRUE(client);

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
       PrefetchTilesCancelOnClientDeletion) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_QUERY});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  std::promise<PrefetchTilesResponse> promise;
  std::future<PrefetchTilesResponse> future = promise.get_future();

  const olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "prefetch-catalog";
  constexpr auto kParitionId = "prefetch-partition";

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, kLayerId, *settings_);
  ASSERT_TRUE(client);

  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile(kParitionId)};
  auto request = olp::dataservice::read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
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
  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataserviceReadVersionedLayerClientTest, PrefetchTilesCancelOnLookup) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_QUERY});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  std::promise<PrefetchTilesResponse> promise;
  std::future<PrefetchTilesResponse> future = promise.get_future();

  const olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "prefetch-catalog";
  constexpr auto kParitionId = "prefetch-partition";

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, kLayerId, *settings_);
  ASSERT_TRUE(client);

  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile(kParitionId)};
  auto request = olp::dataservice::read::PrefetchTilesRequest()
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
  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       PrefetchTilesWithCancellableFuture) {
  olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "hype-test-prefetch";

  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile("5904591")};

  auto request = olp::dataservice::read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, kLayerId, *settings_);
  ASSERT_TRUE(client);

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
  olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "hype-test-prefetch";

  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile("5904591")};

  auto request = olp::dataservice::read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, kLayerId, *settings_);
  ASSERT_TRUE(client);

  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_QUERY});
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

TEST_F(DataserviceReadVersionedLayerClientTest, GetData404Error) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(
      *network_mock_,
      Send(IsGetRequest("https://blob-ireland.data.api.platform.here.com/"
                        "blobstore/v1/catalogs/hereos-internal-test-v2/"
                        "layers/testlayer/data/invalidDataHandle"),
           _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::NOT_FOUND),
                                   "Resource not found."));

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, "testlayer", *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithDataHandle("invalidDataHandle");
  auto future = client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::http::HttpStatusCode::NOT_FOUND,
            data_response.GetError().GetHttpStatusCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetData429Error) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(2)
        .WillRepeatedly(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(
                olp::http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1);
  }

  olp::client::RetrySettings retry_settings;
  retry_settings.retry_condition =
      [](const olp::client::HttpResponse& response) {
        return olp::http::HttpStatusCode::TOO_MANY_REQUESTS == response.status;
      };
  settings_->retry_settings = retry_settings;
  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, "testlayer", *settings_);

  auto request = olp::dataservice::read::DataRequest();
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
  olp::client::HRN hrn(GetTestCatalog());
  {
    testing::InSequence s;
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::FORBIDDEN),
                                     HTTP_RESPONSE_403));
  }

  auto client =
      std::make_unique<VersionedLayerClient>(hrn, "testlayer", *settings_);
  auto request = DataRequest();
  request.WithPartitionId("269");
  // Populate cache
  auto future = client->GetData(request);
  DataResponse data_response = future.GetFuture().get();
  ASSERT_TRUE(data_response.IsSuccessful());
  // Receive 403
  request.WithFetchOption(OnlineOnly);
  future = client->GetData(request);
  data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::http::HttpStatusCode::FORBIDDEN,
            data_response.GetError().GetHttpStatusCode());
  // Check for cached response
  request.WithFetchOption(CacheOnly);
  future = client->GetData(request);
  data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataCacheWithUpdate) {
  olp::client::HRN hrn(GetTestCatalog());
  // Setup the expected calls :
  auto wait_to_start_signal = std::make_shared<std::promise<void>>();
  auto pre_callback_wait = std::make_shared<std::promise<void>>();
  pre_callback_wait->set_value();
  auto wait_for_end_signal = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_to_start_signal, pre_callback_wait,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_BLOB_DATA_269},
      wait_for_end_signal);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  auto client =
      std::make_unique<VersionedLayerClient>(hrn, "testlayer", *settings_);
  auto request = DataRequest();
  request.WithPartitionId("269").WithFetchOption(CacheWithUpdate);
  // Request 1
  auto future = client->GetData(request);
  DataResponse data_response = future.GetFuture().get();
  // Request 1 return. Cached value (nothing)
  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  // Request 2 to check there is a cached value.
  // waiting for cache to fill-in
  wait_for_end_signal->get_future().get();
  request.WithFetchOption(CacheOnly);
  future = client->GetData(request);
  data_response = future.GetFuture().get();
  // Cache should be available here.
  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       CancelPendingRequestsPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  auto client =
      std::make_unique<VersionedLayerClient>(hrn, "testlayer", *settings_);
  auto partitions_request = PartitionsRequest().WithFetchOption(OnlineOnly);
  auto data_request =
      DataRequest().WithPartitionId("269").WithFetchOption(OnlineOnly);

  auto request_started = std::make_shared<std::promise<void>>();
  auto continue_request = std::make_shared<std::promise<void>>();

  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        request_started, continue_request,
        {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_BLOB_DATA_269});

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

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            partitions_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            partitions_response.GetError().GetErrorCode());

  DataResponse data_response = data_future.GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, CancelPendingRequestsPrefetch) {
  olp::client::HRN hrn(GetTestCatalog());

  auto client =
      std::make_unique<VersionedLayerClient>(hrn, "testlayer", *settings_);
  auto prefetch_request = PrefetchTilesRequest();
  auto data_request =
      DataRequest().WithPartitionId("269").WithFetchOption(OnlineOnly);

  auto request_started = std::make_shared<std::promise<void>>();
  auto continue_request = std::make_shared<std::promise<void>>();

  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        request_started, continue_request,
        {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_BLOB_DATA_269});

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

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            prefetch_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            prefetch_response.GetError().GetErrorCode());

  ASSERT_EQ(data_future.wait_for(kWaitTimeout), std::future_status::ready);

  auto data_response = data_future.get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       DISABLED_GetDataWithPartitionIdCancelLookupMetadata) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_METADATA});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, "testlayer", *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       DISABLED_GetDataWithPartitionIdCancelLatestCatalogVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LATEST_CATALOG_VERSION});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .Times(0);

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, "testlayer", *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithPartitionIdCancelLookupQuery) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_QUERY});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
      .Times(0);

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, "testlayer", *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithPartitionIdCancelQuery) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_PARTITION_269});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .Times(0);

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, "testlayer", *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithPartitionIdCancelLookupBlob) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_BLOB});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(0);

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, "testlayer", *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithPartitionIdCancelBlob) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_BLOB_DATA_269});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, "testlayer", *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithPartitionIdVersion2) {
  olp::client::HRN hrn(GetTestCatalog());

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, "testlayer", 2, *settings_);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LAYER_VERSIONS_V2), _, _, _, _))
      .Times(0);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");
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
  olp::client::HRN hrn(GetTestCatalog());

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, "testlayer", 10, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");
  auto data_response = client->GetData(request).GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            data_response.GetError().GetErrorCode());
  ASSERT_EQ(400, data_response.GetError().GetHttpStatusCode());

  request.WithVersion(-1);
  data_response = client->GetData(request).GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            data_response.GetError().GetErrorCode());
  ASSERT_EQ(400, data_response.GetError().GetHttpStatusCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataCacheOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(0);
  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, "testlayer", *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269").WithFetchOption(CacheOnly);
  auto future = client->GetData(request);
  auto data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataOnlineOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(429),
                               "Server busy at the moment."));
  }

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, "testlayer", *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269").WithFetchOption(OnlineOnly);
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
}  // namespace
