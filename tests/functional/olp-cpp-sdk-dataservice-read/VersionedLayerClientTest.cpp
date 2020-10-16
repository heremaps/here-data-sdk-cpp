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

#include <numeric>
#include <string>

#include <gtest/gtest.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/porting/make_unique.h>
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
using VersionedLayerClientTest = VersionedLayerTestBase;

TEST_F(VersionedLayerClientTest, GetPartitions) {
  olp::client::HRN hrn(kTestHrn);
  {
    mock_server_client_->MockAuth();
    mock_server_client_->MockLookupResourceApiResponse(
        mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
            kTestHrn));
    mock_server_client_->MockGetVersionResponse(
        mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));
    mock_server_client_->MockGetResponse(
        mockserver::ReadDefaultResponses::GeneratePartitionsResponse(4),
        url_generator_.PartitionsMetadata());
  }

  auto catalog_client =
      read::VersionedLayerClient(hrn, "testlayer", boost::none, *settings_);

  auto request = read::PartitionsRequest();
  auto future = catalog_client.GetPartitions(request);
  auto partitions_response = future.GetFuture().get();

  EXPECT_SUCCESS(partitions_response);
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
  EXPECT_TRUE(mock_server_client_->Verify());
}

TEST_F(VersionedLayerClientTest, GetAggregatedData) {
  olp::client::HRN hrn(kTestHrn);

  constexpr auto kTileId = "5901734";
  constexpr auto kLayer = "testlayer";
  constexpr auto kQuadTreeDepth = 4;
  constexpr auto kVersion = 44;

  const auto root_tile = olp::geo::TileKey::FromHereTile(kTileId);
  const auto tile = root_tile.ChangedLevelTo(15);
  const auto request = read::TileRequest().WithTileKey(tile);

  // authentification not needed for the test
  settings_->authentication_settings = boost::none;

  {
    SCOPED_TRACE("Requested tile");
    const auto data_handle =
        mockserver::ReadDefaultResponses::GenerateDataHandle(tile.ToHereTile());
    const auto data = mockserver::ReadDefaultResponses::GenerateData();

    {
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
              kTestHrn));
      mock_server_client_->MockGetVersionResponse(
          mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));
      mock_server_client_->MockGetResponse(
          kLayer, root_tile, kVersion,
          mockserver::ReadDefaultResponses::GenerateQuadTreeResponse(
              root_tile, kQuadTreeDepth, {1, 3, 12, 13, 14, 15}));
      mock_server_client_->MockGetResponse(kLayer, data_handle, data);
    }

    auto client =
        read::VersionedLayerClient(hrn, kLayer, boost::none, *settings_);

    auto future = client.GetAggregatedData(request).GetFuture();
    auto response = future.get();
    const auto result = response.MoveResult();
    const auto& result_data = result.GetData();

    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    ASSERT_TRUE(result_data);
    ASSERT_EQ(result_data->size(), data.size());
    EXPECT_TRUE(std::equal(data.begin(), data.end(), result_data->begin()));
    EXPECT_EQ(result.GetTile(), tile);
    EXPECT_TRUE(mock_server_client_->Verify());
  }

  {
    SCOPED_TRACE("Ancestor tile");
    const auto expect_tile = tile.ChangedLevelTo(14);
    const auto data_handle =
        mockserver::ReadDefaultResponses::GenerateDataHandle(
            expect_tile.ToHereTile());
    const auto data = mockserver::ReadDefaultResponses::GenerateData();

    {
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
              kTestHrn));
      mock_server_client_->MockGetVersionResponse(
          mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));
      mock_server_client_->MockGetResponse(
          kLayer, root_tile, kVersion,
          mockserver::ReadDefaultResponses::GenerateQuadTreeResponse(
              root_tile, kQuadTreeDepth, {1, 3, 12, 13, 14}));
      mock_server_client_->MockGetResponse(kLayer, data_handle, data);
    }

    auto client =
        read::VersionedLayerClient(hrn, kLayer, boost::none, *settings_);

    auto future = client.GetAggregatedData(request).GetFuture();
    auto response = future.get();
    const auto result = response.MoveResult();
    const auto& result_data = result.GetData();

    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    ASSERT_TRUE(result.GetData());
    ASSERT_EQ(result_data->size(), data.size());
    EXPECT_TRUE(std::equal(data.begin(), data.end(), result_data->begin()));
    EXPECT_EQ(result.GetTile(), expect_tile);
    EXPECT_TRUE(mock_server_client_->Verify());
  }

  {
    SCOPED_TRACE("Parent tile");
    const auto expect_tile = tile.ChangedLevelTo(3);
    const auto data_handle =
        mockserver::ReadDefaultResponses::GenerateDataHandle(
            expect_tile.ToHereTile());
    const auto data = mockserver::ReadDefaultResponses::GenerateData();

    {
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
              kTestHrn));
      mock_server_client_->MockGetVersionResponse(
          mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));
      mock_server_client_->MockGetResponse(
          kLayer, root_tile, kVersion,
          mockserver::ReadDefaultResponses::GenerateQuadTreeResponse(
              root_tile, kQuadTreeDepth, {1, 2, 3}));
      mock_server_client_->MockGetResponse(kLayer, data_handle, data);
    }

    auto client =
        read::VersionedLayerClient(hrn, kLayer, boost::none, *settings_);

    auto future = client.GetAggregatedData(request).GetFuture();
    auto response = future.get();
    const auto result = response.MoveResult();
    const auto& result_data = result.GetData();

    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    ASSERT_TRUE(result.GetData());
    ASSERT_EQ(result_data->size(), data.size());
    EXPECT_TRUE(std::equal(data.begin(), data.end(), result_data->begin()));
    EXPECT_EQ(result.GetTile(), expect_tile);
    EXPECT_TRUE(mock_server_client_->Verify());
  }

  {
    SCOPED_TRACE("Check cache");
    const auto data_handle =
        mockserver::ReadDefaultResponses::GenerateDataHandle(tile.ToHereTile());
    const auto data = mockserver::ReadDefaultResponses::GenerateData();

    {
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
              kTestHrn));
      mock_server_client_->MockGetVersionResponse(
          mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));
      mock_server_client_->MockGetResponse(
          kLayer, root_tile, kVersion,
          mockserver::ReadDefaultResponses::GenerateQuadTreeResponse(
              root_tile, kQuadTreeDepth, {15}));
      mock_server_client_->MockGetResponse(kLayer, data_handle, data);
    }

    settings_->cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    auto client =
        read::VersionedLayerClient(hrn, kLayer, boost::none, *settings_);

    auto future = client.GetAggregatedData(request).GetFuture();
    auto response = future.get();
    auto result = response.MoveResult();
    auto result_data = result.GetData();

    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    ASSERT_TRUE(result.GetData());
    ASSERT_EQ(result_data->size(), data.size());
    EXPECT_TRUE(std::equal(data.begin(), data.end(), result_data->begin()));
    ASSERT_EQ(result.GetTile(), tile);
    EXPECT_TRUE(mock_server_client_->Verify());

    const auto request =
        read::TileRequest().WithTileKey(tile).WithFetchOption(read::CacheOnly);
    response = client.GetAggregatedData(request).GetFuture().get();
    result = response.MoveResult();
    result_data = result.GetData();

    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    ASSERT_TRUE(result.GetData());
    ASSERT_EQ(result_data->size(), data.size());
    EXPECT_TRUE(std::equal(data.begin(), data.end(), result_data->begin()));
    EXPECT_EQ(result.GetTile(), tile);
    EXPECT_TRUE(mock_server_client_->Verify());
  }
}

}  // namespace
