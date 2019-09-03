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

#include <mocks/NetworkMock.h>
#include <olp/authentication/Settings.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/NetworkRequest.h>
#include <olp/core/http/NetworkResponse.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/dataservice/read/VersionedLayerClient.h>

using namespace olp::dataservice::read;
using namespace olp::tests::common;
using namespace testing;

namespace {

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

constexpr auto kWaitTimeout = std::chrono::seconds(10);

class DataserviceReadVersionedLayerClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    network_mock_ = std::make_shared<NetworkMock>();

    settings_ = std::make_shared<olp::client::OlpClientSettings>();
    settings_->network_request_handler = network_mock_;
    settings_->task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
  }

  void TearDown() override {
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
    network_mock_.reset();
    settings_->task_scheduler.reset();
    settings_.reset();
  }

  std::string GetArgument(const std::string& name) {
    if (name == "dataservice_read_test_catalog") {
      return "hrn:here:data:::here-optimized-map-for-visualization-2";
    } else if (name == "dataservice_read_test_layer") {
      return "omv-base-v2";
    } else if (name == "dataservice_read_test_partition") {
      return "269";
    } else if (name == "dataservice_read_test_layer_version") {
      return "108";
    }
    ADD_FAILURE() << "unknown argument!";
    return "";
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
  auto version =
      std::atoi(GetArgument("dataservice_read_test_layer_version").c_str());

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

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataEmptyPartitionsSync) {
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
      wait_for_cancel, pause_for_cancel, {200, kHttpResponseLookupQuery});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto catalog = olp::client::HRN::FromString(
      GetArgument("dataservice_read_test_catalog"));
  auto layer = GetArgument("dataservice_read_test_layer");
  auto version =
      std::atoi(GetArgument("dataservice_read_test_layer_version").c_str());

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
      wait_for_cancel, pause_for_cancel, {200, kHttpResponsePartition_269});

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
  auto version =
      std::atoi(GetArgument("dataservice_read_test_layer_version").c_str());

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
      wait_for_cancel, pause_for_cancel, {200, kHttpResponseLookupBlob});

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
  auto version =
      std::atoi(GetArgument("dataservice_read_test_layer_version").c_str());

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
      wait_for_cancel, pause_for_cancel, {200, kHttpResponseBlobData_269});

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
  auto version =
      std::atoi(GetArgument("dataservice_read_test_layer_version").c_str());

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

}  // namespace
