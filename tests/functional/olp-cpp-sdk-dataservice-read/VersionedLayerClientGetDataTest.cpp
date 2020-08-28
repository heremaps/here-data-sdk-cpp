/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <gtest/gtest.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
// clang-format off
#include "generated/serializer/PartitionsSerializer.h"
#include "generated/serializer/JsonSerializer.h"
// clang-format on
#include "SetupMockServer.h"
#include "ApiDefaultResponses.h"
#include "MockServerHelper.h"
#include "ReadDefaultResponses.h"
#include "Utils.h"

namespace {

const auto kTestHrn = "hrn:here:data::olp-here-test:hereos-internal-test";
const auto kLayer = "testlayer";
const auto kVersion = 44;
constexpr auto kWaitTimeout = std::chrono::seconds(10);

class VersionedLayerClientGetDataTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();
    settings_ = mockserver::SetupMockServer::CreateSettings(network);
    mock_server_client_ =
        mockserver::SetupMockServer::CreateMockServer(network, kTestHrn);
  }

  void TearDown() override {
    auto network = std::move(settings_->network_request_handler);
    settings_.reset();
    mock_server_client_.reset();
  }

  std::string GenerateGetPartitionsPath(const std::string& hrn,
                                        const std::string& layer) {
    return "/query/v1/catalogs/" + hrn + "/layers/" + layer + "/partitions";
  }

  std::shared_ptr<olp::client::OlpClientSettings> settings_;
  std::shared_ptr<mockserver::MockServerHelper> mock_server_client_;
};

TEST_F(VersionedLayerClientGetDataTest, GetDataFromPartitionSync) {
  olp::client::HRN hrn(kTestHrn);
  auto partition = std::to_string(0);
  const auto data = mockserver::ReadDefaultResponses::GenerateData();
  {
    mock_server_client_->MockAuth();
    mock_server_client_->MockLookupResourceApiResponse(
        mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
            kTestHrn));
    mock_server_client_->MockGetVersionResponse(
        mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));
    mock_server_client_->MockGetResponse(
        mockserver::ReadDefaultResponses::GeneratePartitionsResponse(1),
        GenerateGetPartitionsPath(kTestHrn, kLayer));
    mock_server_client_->MockGetResponse(
        kLayer, mockserver::ReadDefaultResponses::GenerateDataHandle(partition),
        data);
  }

  auto catalog_client = olp::dataservice::read::VersionedLayerClient(
      hrn, kLayer, boost::none, *settings_);

  auto future = catalog_client.GetData(
      olp::dataservice::read::DataRequest().WithPartitionId(partition));
  auto response = future.GetFuture().get();
  EXPECT_SUCCESS(response);
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_EQ(response.GetResult()->size(), data.size());

  EXPECT_TRUE(mock_server_client_->Verify());
}

TEST_F(VersionedLayerClientGetDataTest, GetDataFromPartitionAsync) {
  olp::client::HRN hrn(kTestHrn);
  auto partition = std::to_string(0);
  const auto data = mockserver::ReadDefaultResponses::GenerateData();
  {
    mock_server_client_->MockLookupResourceApiResponse(
        mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
            kTestHrn));
    mock_server_client_->MockGetVersionResponse(
        mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));
    mock_server_client_->MockGetResponse(
        mockserver::ReadDefaultResponses::GeneratePartitionsResponse(1),
        GenerateGetPartitionsPath(kTestHrn, kLayer));
    mock_server_client_->MockGetResponse(
        kLayer, mockserver::ReadDefaultResponses::GenerateDataHandle(partition),
        data);
  }

  auto catalog_client = olp::dataservice::read::VersionedLayerClient(
      hrn, kLayer, boost::none, *settings_);

  std::promise<olp::dataservice::read::DataResponse> promise;
  std::future<olp::dataservice::read::DataResponse> future =
      promise.get_future();

  auto token = catalog_client.GetData(
      olp::dataservice::read::DataRequest().WithPartitionId(partition),
      [&promise](olp::dataservice::read::DataResponse response) {
        promise.set_value(response);
      });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);

  olp::dataservice::read::DataResponse response = future.get();
  EXPECT_SUCCESS(response);
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_EQ(response.GetResult()->size(), data.size());

  EXPECT_TRUE(mock_server_client_->Verify());
}

TEST_F(VersionedLayerClientGetDataTest, GetDataWithHandle) {
  olp::client::HRN hrn(kTestHrn);

  const auto data_handle =
      mockserver::ReadDefaultResponses::GenerateDataHandle("test");
  const auto data = mockserver::ReadDefaultResponses::GenerateData();

  mock_server_client_->MockAuth();
  mock_server_client_->MockLookupResourceApiResponse(
      mockserver::ApiDefaultResponses::GenerateResourceApisResponse(kTestHrn));

  mock_server_client_->MockGetResponse(kLayer, data_handle, data);

  auto client = olp::dataservice::read::VersionedLayerClient(
      hrn, kLayer, boost::none, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithDataHandle(data_handle);

  auto future = client.GetData(request);
  auto data_response = future.GetFuture().get();

  EXPECT_SUCCESS(data_response);
  ASSERT_TRUE(data_response.GetResult() != nullptr);
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ(data, data_string);
  EXPECT_TRUE(mock_server_client_->Verify());
}

TEST_F(VersionedLayerClientGetDataTest, GetDataWithInvalidLayerId) {
  olp::client::HRN hrn(kTestHrn);

  const auto kInvalidLayer = "InvalidLayer";

  mock_server_client_->MockAuth();
  mock_server_client_->MockLookupResourceApiResponse(
      mockserver::ApiDefaultResponses::GenerateResourceApisResponse(kTestHrn));
  mock_server_client_->MockGetVersionResponse(
      mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));
  mock_server_client_->MockGetError(
      olp::client::ApiError(olp::http::HttpStatusCode::BAD_REQUEST),
      GenerateGetPartitionsPath(kTestHrn, kInvalidLayer));

  auto client = olp::dataservice::read::VersionedLayerClient(
      hrn, kInvalidLayer, boost::none, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");

  auto future = client.GetData(request);
  auto data_response = future.GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            data_response.GetError().GetErrorCode());
  EXPECT_TRUE(mock_server_client_->Verify());
}

TEST_F(VersionedLayerClientGetDataTest, GetDataWithPartitionIdVersion2) {
  olp::client::HRN hrn(kTestHrn);

  auto partitions_model =
      mockserver::ReadDefaultResponses::GeneratePartitionsResponse(1);
  auto data_handle = partitions_model.GetPartitions()[0].GetDataHandle();
  auto data = mockserver::ReadDefaultResponses::GenerateData();

  mock_server_client_->MockAuth();
  mock_server_client_->MockLookupResourceApiResponse(
      mockserver::ApiDefaultResponses::GenerateResourceApisResponse(kTestHrn));
  mock_server_client_->MockGetResponse(
      partitions_model, GenerateGetPartitionsPath(kTestHrn, kLayer));

  mock_server_client_->MockGetResponse(kLayer, data_handle, data);

  auto client =
      olp::dataservice::read::VersionedLayerClient(hrn, kLayer, 2, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");

  auto future = client.GetData(request);
  auto data_response = future.GetFuture().get();

  EXPECT_SUCCESS(data_response);
  ASSERT_TRUE(data_response.GetResult() != nullptr);
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ(data, data_string);
  EXPECT_TRUE(mock_server_client_->Verify());
}

}  // namespace
