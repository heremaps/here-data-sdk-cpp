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

#include <string>

#include <gtest/gtest.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
#include "ApiDefaultResponses.h"
#include "ReadDefaultResponses.h"
#include "Utils.h"
#include "VersionedLayerTestBase.h"
// clang-format off
#include "generated/serializer/PartitionsSerializer.h"
#include "generated/serializer/JsonSerializer.h"
#include "SetupMockServer.h"
// clang-format on

namespace {
namespace read = olp::dataservice::read;
constexpr auto kWaitTimeout = std::chrono::seconds(20);
using VersionedLayerClientPrefetchTest = VersionedLayerTestBase;

TEST_F(VersionedLayerClientPrefetchTest, PrefetchTiles) {
  olp::client::HRN hrn(kTestHrn);

  constexpr auto kTileId = "5901734";
  constexpr auto kQuadTreeDepth = 4;

  const auto root_tile = olp::geo::TileKey::FromHereTile(kTileId);
  auto client =
      read::VersionedLayerClient(hrn, kLayer, boost::none, *settings_);
  std::vector<std::string> tiles_data;
  tiles_data.reserve(4);

  {
    SCOPED_TRACE("Prefetch tiles");
    const auto request = read::PrefetchTilesRequest()
                             .WithTileKeys({root_tile})
                             .WithMinLevel(12)
                             .WithMaxLevel(15);
    {
      mock_server_client_->MockAuth();
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
              kTestHrn));
      mock_server_client_->MockGetVersionResponse(
          mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));
      mock_server_client_->MockGetResponse(
          kLayer, root_tile, kVersion,
          mockserver::ReadDefaultResponses::GenerateQuadTreeResponse(
              root_tile, kQuadTreeDepth, {12}));
      const olp::geo::TileKey first_child = root_tile.ChangedLevelBy(1);
      const std::uint64_t begin_tile_key = first_child.ToQuadKey64();

      for (std::uint64_t key = begin_tile_key; key < begin_tile_key + 4;
           ++key) {
        auto child = olp::geo::TileKey::FromQuadKey64(key);
        const auto data_handle =
            mockserver::ReadDefaultResponses::GenerateDataHandle(
                child.ToHereTile());
        tiles_data.emplace_back(
            mockserver::ReadDefaultResponses::GenerateData());

        mock_server_client_->MockGetResponse(kLayer, data_handle,
                                             tiles_data.back());
      }
    }

    auto future = client.PrefetchTiles(request).GetFuture();
    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    const auto result = response.MoveResult();

    EXPECT_EQ(result.size(), 4u);
    for (auto tile_result : result) {
      EXPECT_SUCCESS(*tile_result);
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
    EXPECT_TRUE(mock_server_client_->Verify());
  }

  {
    SCOPED_TRACE("Read cached data from pre-fetched partitions");
    const olp::geo::TileKey first_child = root_tile.ChangedLevelBy(1);
    const std::uint64_t begin_tile_key = first_child.ToQuadKey64();
    auto i = 0;
    for (std::uint64_t key = begin_tile_key; key < begin_tile_key + 4; ++key) {
      auto child = olp::geo::TileKey::FromQuadKey64(key);
      auto future =
          client
              .GetData(read::TileRequest().WithTileKey(child).WithFetchOption(
                  read::CacheOnly))
              .GetFuture();
      auto response = future.get();

      ASSERT_TRUE(response.IsSuccessful())
          << response.GetError().GetMessage().c_str();
      const auto result = response.MoveResult();

      ASSERT_NE(result->size(), 0u);
      std::string data_string(result->begin(), result->end());
      ASSERT_EQ(tiles_data.at(i++), data_string);
    }
  }
  {
    const auto zero_level_tile = root_tile.ChangedLevelTo(0);
    SCOPED_TRACE("Prefetch tiles min/max levels is 0");
    const auto request = read::PrefetchTilesRequest()
                             .WithTileKeys({zero_level_tile})
                             .WithMinLevel(0)
                             .WithMaxLevel(0);
    {
      mock_server_client_->MockGetResponse(
          kLayer, zero_level_tile, kVersion,
          mockserver::ReadDefaultResponses::GenerateQuadTreeResponse(
              zero_level_tile, kQuadTreeDepth, {0, 1}));
      const auto data_handle =
          mockserver::ReadDefaultResponses::GenerateDataHandle(
              zero_level_tile.ToHereTile());

      mock_server_client_->MockGetResponse(
          kLayer, data_handle,
          mockserver::ReadDefaultResponses::GenerateData());
    }

    auto future = client.PrefetchTiles(request).GetFuture();
    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    const auto result = response.MoveResult();

    EXPECT_EQ(result.size(), 1u);
    for (auto tile_result : result) {
      EXPECT_SUCCESS(*tile_result);
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
    EXPECT_TRUE(mock_server_client_->Verify());
  }
  {
    const auto zero_level_tile = root_tile.ChangedLevelTo(0);
    SCOPED_TRACE("Prefetch tiles only min level is 0");
    const auto request = read::PrefetchTilesRequest()
                             .WithTileKeys({zero_level_tile})
                             .WithMinLevel(0)
                             .WithMaxLevel(1);
    {
      // Quad tree and data for tile 1 is in cache, do not need to add mock
      // response
      const olp::geo::TileKey first_child = zero_level_tile.ChangedLevelBy(1);
      const std::uint64_t begin_tile_key = first_child.ToQuadKey64();

      for (std::uint64_t key = begin_tile_key; key < begin_tile_key + 4;
           ++key) {
        auto child = olp::geo::TileKey::FromQuadKey64(key);
        // add mock responce for children
        mock_server_client_->MockGetResponse(
            kLayer,
            mockserver::ReadDefaultResponses::GenerateDataHandle(
                child.ToHereTile()),
            mockserver::ReadDefaultResponses::GenerateData());
      }
    }

    auto future = client.PrefetchTiles(request).GetFuture();
    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    const auto result = response.MoveResult();

    EXPECT_EQ(result.size(), 5u);
    for (auto tile_result : result) {
      EXPECT_SUCCESS(*tile_result);
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
    EXPECT_TRUE(mock_server_client_->Verify());
  }
}

TEST_F(VersionedLayerClientPrefetchTest, PrefetchPartitions) {
  olp::client::HRN hrn(kTestHrn);
  const auto partitions_count = 200u;
  const auto data = mockserver::ReadDefaultResponses::GenerateData();

  auto client =
      read::VersionedLayerClient(hrn, kLayer, boost::none, *settings_);

  {
    SCOPED_TRACE("Prefetch partitions");
    std::vector<std::string> partitions;
    partitions.reserve(partitions_count);
    for (auto i = 0u; i < partitions_count; i++) {
      partitions.emplace_back(std::to_string(i));
    }
    const auto request =
        read::PrefetchPartitionsRequest().WithPartitionIds(partitions);
    {
      mock_server_client_->MockAuth();
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
              kTestHrn));
      mock_server_client_->MockGetVersionResponse(
          mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));

      mock_server_client_->MockGetResponse(
          mockserver::ReadDefaultResponses::GeneratePartitionsResponse(
              partitions_count / 2),
          url_generator_.PartitionsQuery());

      mock_server_client_->MockGetError(
          {olp::http::HttpStatusCode::NOT_FOUND, "Not found"},
          url_generator_.PartitionsQuery());

      for (auto i = 0u; i < partitions_count / 2; i++) {
        auto data_handle =
            mockserver::ReadDefaultResponses::GenerateDataHandle(partitions[i]);
        mock_server_client_->MockGetResponse(kLayer, data_handle, data);
      }
    }
    std::promise<read::PrefetchPartitionsResponse> promise;
    auto future = promise.get_future();
    auto token = client.PrefetchPartitions(
        request, [&promise](read::PrefetchPartitionsResponse response) {
          promise.set_value(std::move(response));
        });
    EXPECT_EQ(future.wait_for(kWaitTimeout), std::future_status::ready);

    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful());
    const auto result = response.MoveResult();

    EXPECT_EQ(result.GetPartitions().size(), partitions_count / 2);
    for (const auto& partition : result.GetPartitions()) {
      ASSERT_TRUE(client.IsCached(partition));
      auto data_request =
          read::DataRequest().WithPartitionId(partition).WithFetchOption(
              read::FetchOptions::CacheOnly);
      auto data_response = client.GetData(data_request).GetFuture().get();
      ASSERT_TRUE(data_response.IsSuccessful());
      EXPECT_EQ(data_response.GetResult()->size(), data.size());
    }
  }
  {
    SCOPED_TRACE("Failed request");
    const auto request =
        read::PrefetchPartitionsRequest().WithPartitionIds({"201"});
    {
      mock_server_client_->MockGetError(
          {olp::http::HttpStatusCode::NOT_FOUND, "Not found"},
          url_generator_.PartitionsQuery());
    }

    auto future = client.PrefetchPartitions(request).GetFuture();
    EXPECT_EQ(future.wait_for(kWaitTimeout), std::future_status::ready);
    auto response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(olp::client::ErrorCode::NotFound,
              response.GetError().GetErrorCode());
  }
  {
    SCOPED_TRACE("Empty json responce");
    const auto request =
        read::PrefetchPartitionsRequest().WithPartitionIds({"201"});
    {
      mock_server_client_->MockGetError({olp::http::HttpStatusCode::OK, ""},
                                        url_generator_.PartitionsQuery());
    }

    auto future = client.PrefetchPartitions(request).GetFuture();
    EXPECT_EQ(future.wait_for(kWaitTimeout), std::future_status::ready);
    auto response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(olp::client::ErrorCode::Unknown,
              response.GetError().GetErrorCode());
    ASSERT_EQ("Fail parsing response.", response.GetError().GetMessage());
  }
  {
    SCOPED_TRACE("Download all data handles fails");
    std::vector<std::string> partitions;
    partitions.reserve(partitions_count);
    for (auto i = partitions_count + 1; i < partitions_count + 11; i++) {
      partitions.emplace_back(std::to_string(i));
    }

    const auto request =
        read::PrefetchPartitionsRequest().WithPartitionIds(partitions);
    {
      mock_server_client_->MockGetResponse(
          mockserver::ReadDefaultResponses::GeneratePartitionsResponse(
              10, partitions_count + 1),
          url_generator_.PartitionsQuery());

      for (auto partition : partitions) {
        auto data_handle =
            mockserver::ReadDefaultResponses::GenerateDataHandle(partition);
        mock_server_client_->MockGetError(
            {olp::http::HttpStatusCode::NOT_FOUND, "Not found"},
            url_generator_.DataBlob(data_handle));
      }
    }

    auto future = client.PrefetchPartitions(request).GetFuture();
    EXPECT_EQ(future.wait_for(kWaitTimeout), std::future_status::ready);
    auto response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(olp::client::ErrorCode::Unknown,
              response.GetError().GetErrorCode());
    ASSERT_EQ("No partitions were prefetched.",
              response.GetError().GetMessage());
  }
  {
    SCOPED_TRACE("Download some data handles fails");
    std::vector<std::string> partitions;
    partitions.reserve(partitions_count);
    for (auto i = partitions_count + 1; i < partitions_count + 11; i++) {
      partitions.emplace_back(std::to_string(i));
    }

    const auto request =
        read::PrefetchPartitionsRequest().WithPartitionIds(partitions);
    {
      for (auto i = 0u; i < partitions.size(); i++) {
        auto data_handle =
            mockserver::ReadDefaultResponses::GenerateDataHandle(partitions[i]);
        if (i % 2 == 0) {
          mock_server_client_->MockGetResponse(kLayer, data_handle, data);
        } else {
          mock_server_client_->MockGetError(
              {olp::http::HttpStatusCode::NOT_FOUND, "Not found"},
              url_generator_.DataBlob(data_handle));
        }
      }
    }

    auto future = client.PrefetchPartitions(request).GetFuture();
    EXPECT_EQ(future.wait_for(kWaitTimeout), std::future_status::ready);
    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful());
    const auto result = response.MoveResult();
    for (const auto& partition : result.GetPartitions()) {
      ASSERT_TRUE(client.IsCached(partition));
      auto data_request =
          read::DataRequest().WithPartitionId(partition).WithFetchOption(
              read::FetchOptions::CacheOnly);
      auto data_response = client.GetData(data_request).GetFuture().get();
      ASSERT_TRUE(data_response.IsSuccessful());
      EXPECT_EQ(data_response.GetResult()->size(), data.size());
    }
  }
  EXPECT_TRUE(mock_server_client_->Verify());
}
}  // namespace
