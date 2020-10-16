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
#include <olp/dataservice/read/VersionedLayerClient.h>
#include "ApiDefaultResponses.h"
#include "ReadDefaultResponses.h"
#include "Utils.h"
#include "VersionedLayerTestBase.h"
// clang-format off
#include "generated/serializer/PartitionsSerializer.h"
#include "generated/serializer/JsonSerializer.h"
#include "MockServerHelper.h"
// clang-format on

namespace {
namespace read = olp::dataservice::read;
constexpr auto kWaitTimeout = std::chrono::seconds(10);
using VersionedLayerClientGetDataTest = VersionedLayerTestBase;

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
        url_generator_.PartitionsQuery());
    mock_server_client_->MockGetResponse(
        kLayer, mockserver::ReadDefaultResponses::GenerateDataHandle(partition),
        data);
  }

  auto catalog_client =
      read::VersionedLayerClient(hrn, kLayer, boost::none, *settings_);

  auto future =
      catalog_client.GetData(read::DataRequest().WithPartitionId(partition));
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
        url_generator_.PartitionsQuery());
    mock_server_client_->MockGetResponse(
        kLayer, mockserver::ReadDefaultResponses::GenerateDataHandle(partition),
        data);
  }

  auto catalog_client =
      read::VersionedLayerClient(hrn, kLayer, boost::none, *settings_);

  std::promise<read::DataResponse> promise;
  std::future<read::DataResponse> future = promise.get_future();

  auto token = catalog_client.GetData(
      read::DataRequest().WithPartitionId(partition),
      [&promise](read::DataResponse response) { promise.set_value(response); });
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);

  read::DataResponse response = future.get();
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

  auto client =
      read::VersionedLayerClient(hrn, kLayer, boost::none, *settings_);

  auto request = read::DataRequest();
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

  mock_server_client_->MockAuth();
  mock_server_client_->MockLookupResourceApiResponse(
      mockserver::ApiDefaultResponses::GenerateResourceApisResponse(kTestHrn));
  mock_server_client_->MockGetVersionResponse(
      mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));
  mock_server_client_->MockGetError(
      olp::client::ApiError(olp::http::HttpStatusCode::BAD_REQUEST),
      url_generator_.PartitionsQuery());

  auto client =
      read::VersionedLayerClient(hrn, kLayer, boost::none, *settings_);

  auto request = read::DataRequest();
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
  mock_server_client_->MockGetResponse(partitions_model,
                                       url_generator_.PartitionsQuery());

  mock_server_client_->MockGetResponse(kLayer, data_handle, data);

  auto client = read::VersionedLayerClient(hrn, kLayer, 2, *settings_);

  auto request = read::DataRequest();
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

TEST_F(VersionedLayerClientGetDataTest,
       GetDataFromPartitionLatestVersionAsync) {
  const olp::client::HRN kHrn(kTestHrn);
  const auto kPartitionName = "269";
  const std::string tile_data =
      mockserver::ReadDefaultResponses::GenerateData();

  auto partition = mockserver::ReadDefaultResponses::GeneratePartitionResponse(
      kPartitionName);
  auto data_handle = partition.GetDataHandle();
  read::model::Partitions partitions;
  partitions.SetPartitions({partition});

  {
    mock_server_client_->MockAuth();
    mock_server_client_->MockLookupResourceApiResponse(
        mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
            kTestHrn));
    mock_server_client_->MockGetVersionResponse(
        mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));
    mock_server_client_->MockGetResponse(partitions,
                                         url_generator_.PartitionsQuery());
    mock_server_client_->MockGetResponse(kLayer, data_handle, tile_data);
  }

  read::VersionedLayerClient client(kHrn, kLayer, boost::none, *settings_);

  std::promise<read::DataResponse> promise;
  std::future<read::DataResponse> future = promise.get_future();

  auto token = client.GetData(
      read::DataRequest().WithPartitionId(kPartitionName),
      [&promise](read::DataResponse response) { promise.set_value(response); });

  auto response = future.get();
  auto result = response.GetResult();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_EQ(result->size(), tile_data.size());
  EXPECT_TRUE(std::equal(tile_data.begin(), tile_data.end(), result->begin()));
  EXPECT_TRUE(mock_server_client_->Verify());
}

TEST_F(VersionedLayerClientGetDataTest, GetDataWithInvalidDataHandle) {
  const olp::client::HRN kHrn(kTestHrn);
  constexpr auto kDataHandle = "invalidDataHandle";

  {
    mock_server_client_->MockAuth();
    mock_server_client_->MockLookupResourceApiResponse(
        mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
            kTestHrn));
    mock_server_client_->MockGetError(
        {olp::http::HttpStatusCode::NOT_FOUND, "Not found"},
        url_generator_.DataBlob(kDataHandle));
  }

  read::VersionedLayerClient client(kHrn, kLayer, boost::none, *settings_);

  auto request = read::DataRequest();
  request.WithDataHandle(kDataHandle);
  auto future = client.GetData(request).GetFuture();
  auto response = future.get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(olp::http::HttpStatusCode::NOT_FOUND,
            response.GetError().GetHttpStatusCode());
  EXPECT_TRUE(mock_server_client_->Verify());
}

}  // namespace
