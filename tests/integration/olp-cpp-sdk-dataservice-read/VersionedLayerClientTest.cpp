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
#include <olp/core/http/NetworkRequest.h>
#include <olp/core/http/NetworkResponse.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
#include "HttpResponses.h"

using namespace olp::dataservice::read;
using namespace olp::tests::common;
using namespace testing;

namespace {

std::string GetArgument(const std::string& name) {
  if (name == "dataservice_read_test_catalog") {
    return "hrn:here:data:::here-optimized-map-for-visualization-2";
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

constexpr char kHttpResponseLookupQuery[] =
    R"jsonString([{"api":"query","version":"v1","baseURL":"https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";

constexpr char kHttpResponsePartition_269[] =
    R"jsonString({ "partitions": [{"version":4,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"}]})jsonString";

constexpr char kHttpResponsePartitionsEmpty[] =
    R"jsonString({ "partitions": []})jsonString";

constexpr char kHttpResponseLookupBlob[] =
    R"jsonString([{"api":"blob","version":"v1","baseURL":"https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";

constexpr char kHttpResponseBlobData_269[] =
    R"jsonString(DT_2_0031)jsonString";

constexpr char kHttpResponseLatestCatalogVersion[] =
    R"jsonString({"version":4})jsonString";

constexpr auto kWaitTimeout = std::chrono::seconds(1);

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

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_PARTITION_269));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_LOOKUP_BLOB));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_BLOB_DATA_269));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITION_3), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_PARTITION_3));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_LOOKUP_VOLATILE_BLOB));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_LAYER_VERSIONS_V2), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_LAYER_VERSIONS_V2));

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS_V2), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_PARTITIONS_V2));

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

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG_V2), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_CONFIG_V2));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_QUADKEYS_23618364), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_QUADKEYS_23618364));

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

    ON_CALL(*network_mock_, Send(IsGetRequest(URL_QUADKEYS_369036), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_QUADKEYS_369036));

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
            Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_3), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_BLOB_DATA_PREFETCH_3));

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

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_7), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               HTTP_RESPONSE_BLOB_DATA_PREFETCH_7));

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

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);
  ASSERT_TRUE(catalog_client);

  std::promise<VersionedLayerClient::CallbackResponse> promise;
  std::future<VersionedLayerClient::CallbackResponse> future =
      promise.get_future();
  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition),
      [&promise](VersionedLayerClient::CallbackResponse response) {
        promise.set_value(response);
      });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::CallbackResponse response = future.get();

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
  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, sync_settings);
  ASSERT_TRUE(catalog_client);

  VersionedLayerClient::CallbackResponse response;

  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition),
      [&response](VersionedLayerClient::CallbackResponse resp) {
        response = std::move(resp);
      });
  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult() != nullptr);
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
  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, sync_settings);
  ASSERT_TRUE(catalog_client);

  VersionedLayerClient::CallbackResponse response;

  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(boost::none)
          .WithPartitionId(partition),
      [&response](VersionedLayerClient::CallbackResponse resp) {
        response = std::move(resp);
      });
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
  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, sync_settings);
  ASSERT_TRUE(catalog_client);

  VersionedLayerClient::CallbackResponse response;

  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(boost::none)
          .WithPartitionId(partition),
      [&response](VersionedLayerClient::CallbackResponse resp) {
        response = std::move(resp);
      });
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
  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, sync_settings);
  ASSERT_TRUE(catalog_client);

  VersionedLayerClient::CallbackResponse response;

  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition)
          .WithFetchOption(FetchOptions::CacheWithUpdate),
      [&response](VersionedLayerClient::CallbackResponse resp) {
        response = std::move(resp);
      });
  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_FALSE(response.GetResult() != nullptr);

  token = catalog_client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition)
          .WithFetchOption(FetchOptions::CacheOnly),
      [&response](VersionedLayerClient::CallbackResponse resp) {
        response = std::move(resp);
      });
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
  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, sync_settings);
  ASSERT_TRUE(catalog_client);

  VersionedLayerClient::CallbackResponse response;

  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition),
      [&response](VersionedLayerClient::CallbackResponse resp) {
        response = std::move(resp);
      });
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

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);
  ASSERT_TRUE(catalog_client);

  std::promise<VersionedLayerClient::CallbackResponse> promise;
  std::future<VersionedLayerClient::CallbackResponse> future =
      promise.get_future();
  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition),
      [&promise](VersionedLayerClient::CallbackResponse response) {
        promise.set_value(response);
      });

  wait_for_cancel->get_future().get();
  token.cancel();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::CallbackResponse response = future.get();

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

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);
  ASSERT_TRUE(catalog_client);

  std::promise<VersionedLayerClient::CallbackResponse> promise;
  std::future<VersionedLayerClient::CallbackResponse> future =
      promise.get_future();
  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition),
      [&promise](VersionedLayerClient::CallbackResponse response) {
        promise.set_value(response);
      });

  wait_for_cancel->get_future().get();
  token.cancel();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::CallbackResponse response = future.get();

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

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);
  ASSERT_TRUE(catalog_client);

  std::promise<VersionedLayerClient::CallbackResponse> promise;
  std::future<VersionedLayerClient::CallbackResponse> future =
      promise.get_future();
  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition),
      [&promise](VersionedLayerClient::CallbackResponse response) {
        promise.set_value(response);
      });

  wait_for_cancel->get_future().get();
  token.cancel();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::CallbackResponse response = future.get();

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

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);
  ASSERT_TRUE(catalog_client);

  std::promise<VersionedLayerClient::CallbackResponse> promise;
  std::future<VersionedLayerClient::CallbackResponse> future =
      promise.get_future();
  auto partition = GetArgument("dataservice_read_test_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition)
          .WithFetchOption(FetchOptions::CacheWithUpdate),
      [&promise](VersionedLayerClient::CallbackResponse response) {
        promise.set_value(response);
      });

  wait_for_cancel->get_future().get();
  token.cancel();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::CallbackResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_TRUE(response.GetResult() == nullptr);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsNoError) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  std::promise<VersionedLayerClient::PartitionsResponse> promise;
  auto future = promise.get_future();
  auto token = catalog_client->GetPartitions(
      request, [&promise](VersionedLayerClient::PartitionsResponse response) {
        promise.set_value(response);
      });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::PartitionsResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(4u, response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetEmptyPartitions) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_EMPTY_PARTITIONS));

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  std::promise<VersionedLayerClient::PartitionsResponse> promise;
  auto future = promise.get_future();
  auto token = catalog_client->GetPartitions(
      request, [&promise](VersionedLayerClient::PartitionsResponse response) {
        promise.set_value(response);
      });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::PartitionsResponse response = future.get();

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
        return 429 == response.status;
      };
  settings_->retry_settings = retry_settings;
  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  std::promise<VersionedLayerClient::PartitionsResponse> promise;
  auto future = promise.get_future();
  auto token = catalog_client->GetPartitions(
      request, [&promise](VersionedLayerClient::PartitionsResponse response) {
        promise.set_value(response);
      });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::PartitionsResponse response = future.get();

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
        return 429 == response.status;
      };
  settings_->retry_settings = retry_settings;
  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  std::promise<VersionedLayerClient::PartitionsResponse> promise;
  auto future = promise.get_future();
  auto token = catalog_client->GetPartitions(
      request, [&promise](VersionedLayerClient::PartitionsResponse response) {
        promise.set_value(response);
      });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::PartitionsResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(4u, response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsForInvalidLayer) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = "somewhat_not_okay";

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  std::promise<VersionedLayerClient::PartitionsResponse> promise;
  auto future = promise.get_future();
  auto token = catalog_client->GetPartitions(
      request, [&promise](VersionedLayerClient::PartitionsResponse response) {
        promise.set_value(response);
      });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::PartitionsResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsCacheWithUpdate) {
  olp::logging::Log::setLevel(olp::logging::Level::Trace);

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
      wait_to_start_signal, pre_callback_wait, {200, HTTP_RESPONSE_PARTITIONS},
      wait_for_end_signal);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  // Request 1
  {
    std::promise<VersionedLayerClient::PartitionsResponse> promise;
    auto future = promise.get_future();
    auto request = PartitionsRequest().WithFetchOption(CacheWithUpdate);
    auto token = catalog_client->GetPartitions(
        request, [&promise](VersionedLayerClient::PartitionsResponse response) {
          promise.set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    VersionedLayerClient::PartitionsResponse response = future.get();
    // Request 1 return. Cached value (nothing)
    ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  }

  wait_for_end_signal->get_future().get();

  // Request 2
  {
    std::promise<VersionedLayerClient::PartitionsResponse> promise;
    auto future = promise.get_future();
    auto request = PartitionsRequest().WithFetchOption(CacheOnly);
    auto token = catalog_client->GetPartitions(
        request, [&promise](VersionedLayerClient::PartitionsResponse response) {
          promise.set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    VersionedLayerClient::PartitionsResponse response = future.get();
    // Cache should be available here.
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitions403CacheClear) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
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
    std::promise<VersionedLayerClient::PartitionsResponse> promise;
    auto future = promise.get_future();
    auto token = catalog_client->GetPartitions(
        request, [&promise](VersionedLayerClient::PartitionsResponse response) {
          promise.set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    VersionedLayerClient::PartitionsResponse response = future.get();

    ASSERT_TRUE(response.IsSuccessful());
  }

  // Receive 403
  {
    request.WithFetchOption(OnlineOnly);

    std::promise<VersionedLayerClient::PartitionsResponse> promise;
    auto future = promise.get_future();
    auto token = catalog_client->GetPartitions(
        request, [&promise](VersionedLayerClient::PartitionsResponse response) {
          promise.set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    VersionedLayerClient::PartitionsResponse response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(403, response.GetError().GetHttpStatusCode());
  }

  // Check for cached response
  {
    request.WithFetchOption(CacheOnly);
    std::promise<VersionedLayerClient::PartitionsResponse> promise;
    auto future = promise.get_future();
    auto token = catalog_client->GetPartitions(
        request, [&promise](VersionedLayerClient::PartitionsResponse response) {
          promise.set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    VersionedLayerClient::PartitionsResponse response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsGarbageResponse) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   R"jsonString(kd3sdf\)jsonString"));

  auto request = olp::dataservice::read::PartitionsRequest();
  std::promise<VersionedLayerClient::PartitionsResponse> promise;
  auto future = promise.get_future();
  auto token = catalog_client->GetPartitions(
      request, [&promise](VersionedLayerClient::PartitionsResponse response) {
        promise.set_value(response);
      });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::PartitionsResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::ServiceUnavailable,
            response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetPartitionsCancelLookupMetadata) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_METADATA});

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
  std::promise<VersionedLayerClient::PartitionsResponse> promise;
  auto future = promise.get_future();
  auto token = catalog_client->GetPartitions(
      request, [&promise](VersionedLayerClient::PartitionsResponse response) {
        promise.set_value(response);
      });

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  token.cancel();
  pause_for_cancel->set_value();  // unblock the handler

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::PartitionsResponse response = future.get();

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

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) =
      GenerateNetworkMockActions(wait_for_cancel, pause_for_cancel,
                                 {200, HTTP_RESPONSE_LATEST_CATALOG_VERSION});

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
  std::promise<VersionedLayerClient::PartitionsResponse> promise;
  auto future = promise.get_future();
  auto token = catalog_client->GetPartitions(
      request, [&promise](VersionedLayerClient::PartitionsResponse response) {
        promise.set_value(response);
      });

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  token.cancel();
  pause_for_cancel->set_value();  // unblock the handler

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::PartitionsResponse response = future.get();

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

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LAYER_VERSIONS});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(0);

  auto request = olp::dataservice::read::PartitionsRequest();
  std::promise<VersionedLayerClient::PartitionsResponse> promise;
  auto future = promise.get_future();
  auto token = catalog_client->GetPartitions(
      request, [&promise](VersionedLayerClient::PartitionsResponse response) {
        promise.set_value(response);
      });

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  token.cancel();
  pause_for_cancel->set_value();  // unblock the handler

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::PartitionsResponse response = future.get();

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

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LAYER_VERSIONS_V2), _, _, _, _))
      .Times(1);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithVersion(2);
  std::promise<VersionedLayerClient::PartitionsResponse> promise;
  auto future = promise.get_future();
  auto token = catalog_client->GetPartitions(
      request, [&promise](VersionedLayerClient::PartitionsResponse response) {
        promise.set_value(response);
      });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::PartitionsResponse response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(1u, response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsInvalidVersion) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  {
    request.WithVersion(10);
    std::promise<VersionedLayerClient::PartitionsResponse> promise;
    auto future = promise.get_future();
    auto token = catalog_client->GetPartitions(
        request, [&promise](VersionedLayerClient::PartitionsResponse response) {
          promise.set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    VersionedLayerClient::PartitionsResponse response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(olp::client::ErrorCode::BadRequest,
              response.GetError().GetErrorCode());
    ASSERT_EQ(400, response.GetError().GetHttpStatusCode());
  }

  {
    request.WithVersion(-1);
    std::promise<VersionedLayerClient::PartitionsResponse> promise;
    auto future = promise.get_future();
    auto token = catalog_client->GetPartitions(
        request, [&promise](VersionedLayerClient::PartitionsResponse response) {
          promise.set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    VersionedLayerClient::PartitionsResponse response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(olp::client::ErrorCode::BadRequest,
              response.GetError().GetErrorCode());
    ASSERT_EQ(400, response.GetError().GetHttpStatusCode());
    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(olp::client::ErrorCode::BadRequest,
              response.GetError().GetErrorCode());
    ASSERT_EQ(400, response.GetError().GetHttpStatusCode());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsCacheOnly) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(0);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithFetchOption(CacheOnly);
  std::promise<VersionedLayerClient::PartitionsResponse> promise;
  auto future = promise.get_future();
  auto token = catalog_client->GetPartitions(
      request, [&promise](VersionedLayerClient::PartitionsResponse response) {
        promise.set_value(response);
      });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::PartitionsResponse response = future.get();

  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsOnlineOnly) {
  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(429),
                               "Server busy at the moment."));
  }

  auto request = olp::dataservice::read::PartitionsRequest();
  {
    std::promise<VersionedLayerClient::PartitionsResponse> promise;
    auto future = promise.get_future();
    auto token = catalog_client->GetPartitions(
        request, [&promise](VersionedLayerClient::PartitionsResponse response) {
          promise.set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    VersionedLayerClient::PartitionsResponse response = future.get();

    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_EQ(4u, response.GetResult().GetPartitions().size());
  }

  {
    request.WithFetchOption(OnlineOnly);
    std::promise<VersionedLayerClient::PartitionsResponse> promise;
    auto future = promise.get_future();
    auto token = catalog_client->GetPartitions(
        request, [&promise](VersionedLayerClient::PartitionsResponse response) {
          promise.set_value(response);
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    VersionedLayerClient::PartitionsResponse response = future.get();

    // Should fail despite valid cache entry
    ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  }
}

}  // namespace
