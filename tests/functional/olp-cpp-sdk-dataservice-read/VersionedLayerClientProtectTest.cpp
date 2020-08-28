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
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/utils/Dir.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
#include <testutils/CustomParameters.hpp>
#include "ApiDefaultResponses.h"
#include "MockServerHelper.h"
#include "ReadDefaultResponses.h"
#include "SetupMockServer.h"
#include "Utils.h"

namespace {

const auto kTestHrn = "hrn:here:data::olp-here-test:hereos-internal-test";

class VersionedLayerClientProtectTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();

    settings_ = std::make_shared<olp::client::OlpClientSettings>();
    olp::cache::CacheSettings cache_settings;
    cache_path_ = olp::utils::Dir::TempDirectory() + "/test";
    cache_settings.disk_path_mutable = cache_path_;
    olp::utils::Dir::remove(cache_path_);
    cache_settings.max_memory_cache_size = 0u;
    cache_settings.eviction_policy =
        olp::cache::EvictionPolicy::kLeastRecentlyUsed;
    cache_settings.max_disk_storage =
        46484u / 0.85;  // eviction trashold is (0.8500000238F)
    settings_->cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache(
            cache_settings);
    settings_->network_request_handler = network;
    // setup proxy
    settings_->proxy_settings =
        mockserver::SetupMockServer::CreateProxySettings();
    mock_server_client_ =
        mockserver::SetupMockServer::CreateMockServer(network, kTestHrn);
  }

  void TearDown() override {
    auto network = std::move(settings_->network_request_handler);
    settings_.reset();
    mock_server_client_.reset();
    olp::utils::Dir::remove(cache_path_);
  }

  std::shared_ptr<olp::client::OlpClientSettings> settings_;
  std::shared_ptr<mockserver::MockServerHelper> mock_server_client_;
  std::string cache_path_;
};

TEST_F(VersionedLayerClientProtectTest, ProtectAndReleaseWithEviction) {
  olp::client::HRN hrn(kTestHrn);

  constexpr auto kTileId = "5901734";
  constexpr auto kLayer = "testlayer";
  constexpr auto kQuadTreeDepth = 4;
  constexpr auto kVersion = 44;

  const auto base_tile = olp::geo::TileKey::FromHereTile(kTileId);
  std::vector<olp::geo::TileKey> request_tiles;
  request_tiles.reserve(3);
  const std::uint64_t first_tile_key = base_tile.ToQuadKey64();
  for (std::uint64_t key = first_tile_key; key < first_tile_key + 3; ++key) {
    auto child = olp::geo::TileKey::FromQuadKey64(key);
    request_tiles.push_back(child);
  }

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, kLayer, boost::none, *settings_);

  auto check_if_tiles_cached = [&](const std::vector<olp::geo::TileKey>& tiles,
                                   bool expected_result) {
    for (const auto& tile : tiles) {
      ASSERT_EQ(client->IsCached(tile), expected_result);
    }
  };

  std::vector<olp::geo::TileKey> tiles_lover_levels;
  std::vector<olp::geo::TileKey> tiles_upper_levels;
  tiles_lover_levels.reserve(192);
  tiles_upper_levels.reserve(48);

  {
    mock_server_client_->MockLookupResourceApiResponse(
        mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
            kTestHrn));
    mock_server_client_->MockGetVersionResponse(
        mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));

    for (const auto& base_tile : request_tiles) {
      const std::uint64_t first_tile_key =
          base_tile.ChangedLevelBy(1).ToQuadKey64();
      for (std::uint64_t key = first_tile_key; key < first_tile_key + 4;
           ++key) {
        auto child = olp::geo::TileKey::FromQuadKey64(key);
        mock_server_client_->MockGetResponse(
            kLayer, child, kVersion,
            mockserver::ReadDefaultResponses::GenerateQuadTreeResponse(
                child, kQuadTreeDepth, {13, 14}));
      }

      auto levels_changed = 2;
      while (levels_changed < 4) {
        const olp::geo::TileKey first_child =
            base_tile.ChangedLevelBy(levels_changed);
        const std::uint64_t begin_tile_key = first_child.ToQuadKey64();
        int childCount =
            olp::geo::QuadKey64Helper::ChildrenAtLevel(levels_changed);
        for (std::uint64_t key = begin_tile_key;
             key < begin_tile_key + childCount; ++key) {
          auto child = olp::geo::TileKey::FromQuadKey64(key);
          const auto data_handle =
              mockserver::ReadDefaultResponses::GenerateDataHandle(
                  child.ToHereTile());
          if (child.Level() < 14) {
            tiles_upper_levels.emplace_back(std::move(child));
          } else {
            tiles_lover_levels.emplace_back(std::move(child));
          }
          mock_server_client_->MockGetResponse(
              kLayer, data_handle,
              mockserver::ReadDefaultResponses::GenerateData());
        }
        levels_changed++;
      }
    }
  }

  {
    SCOPED_TRACE("Prefetch tiles for levels 14 and 16");
    const auto request = olp::dataservice::read::PrefetchTilesRequest()
                             .WithTileKeys(request_tiles)
                             .WithMinLevel(14)
                             .WithMaxLevel(16);
    auto future = client->PrefetchTiles(request).GetFuture();
    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    const auto result = response.MoveResult();

    EXPECT_EQ(result.size(), 192);
    for (auto tile_result : result) {
      EXPECT_SUCCESS(*tile_result);
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }
  {
    SCOPED_TRACE("Protect tiles");
    auto startTime = std::chrono::high_resolution_clock::now();
    auto protect_response = client->Protect(tiles_lover_levels);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time = end - startTime;
    std::cout
        << "Protect duration: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(time).count()
        << " ms" << std::endl;
    // protect api to not have multiple requests
    ASSERT_TRUE(
        settings_->cache->Protect({"hrn:here:data::olp-here-test:hereos-"
                                   "internal-test::blob::v1::api"}));
    ASSERT_TRUE(protect_response);
  }
  {
    SCOPED_TRACE("Prefetch tiles for levels left, some data will be evicted");
    const auto request = olp::dataservice::read::PrefetchTilesRequest()
                             .WithTileKeys(request_tiles)
                             .WithMinLevel(13)
                             .WithMaxLevel(16);
    auto future = client->PrefetchTiles(request).GetFuture();
    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    const auto result = response.MoveResult();

    for (auto tile_result : result) {
      EXPECT_SUCCESS(*tile_result);
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }
  {
    SCOPED_TRACE("Protected tiles not evicted");
    check_if_tiles_cached(tiles_lover_levels, true);
  }
  {
    SCOPED_TRACE("Tiles which is not protected was evicted");
    // last prefetched element is not evicted
    tiles_upper_levels.erase(--tiles_upper_levels.end());
    check_if_tiles_cached(tiles_upper_levels, false);
  }
  {
    SCOPED_TRACE("Release tiles");
    auto startTime = std::chrono::high_resolution_clock::now();
    auto release_response = client->Release(tiles_lover_levels);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time = end - startTime;
    std::cout
        << "Release duration: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(time).count()
        << " ms" << std::endl;
    ASSERT_TRUE(release_response);
  }
  {
    SCOPED_TRACE("Get tile to force eviction");
    const auto data_handle =
        mockserver::ReadDefaultResponses::GenerateDataHandle(
            tiles_upper_levels.front().ToHereTile());
    mock_server_client_->MockGetResponse(
        kLayer, data_handle, mockserver::ReadDefaultResponses::GenerateData());

    const auto request = olp::dataservice::read::TileRequest().WithTileKey(
        tiles_upper_levels.front());
    auto future = client->GetData(request).GetFuture();
    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
  }
  {
    SCOPED_TRACE("Check that released key is evicted");
    ASSERT_FALSE(client->IsCached(tiles_lover_levels.front()));
  }
}

TEST_F(VersionedLayerClientProtectTest, OverlappedQuads) {
  olp::client::HRN hrn(kTestHrn);

  constexpr auto kTileId = "5901734";
  constexpr auto kLayer = "testlayer";
  constexpr auto kQuadTreeDepth = 4;
  constexpr auto kVersion = 44;

  const auto base_tile = olp::geo::TileKey::FromHereTile(kTileId);

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, kLayer, boost::none, *settings_);

  auto check_if_quads_protected =
      [&](const std::vector<olp::geo::TileKey>& tiles, bool expected_result) {
        for (const auto& tile : tiles) {
          auto key =
              "hrn:here:data::olp-here-test:hereos-internal-test::testlayer::" +
              tile.ToHereTile() + "::44::4::quadtree";
          ASSERT_EQ(settings_->cache->IsProtected(key), expected_result);
        }
      };

  std::vector<olp::geo::TileKey> tiles_prefetched;
  std::vector<olp::geo::TileKey> tiles_to_protect;

  {
    mock_server_client_->MockLookupResourceApiResponse(
        mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
            kTestHrn));
    mock_server_client_->MockGetVersionResponse(
        mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));

    // quad for 12 level
    const std::uint64_t first_tile_key =
        base_tile.ChangedLevelBy(1).ToQuadKey64();
    for (std::uint64_t key = first_tile_key; key < first_tile_key + 4; ++key) {
      auto child = olp::geo::TileKey::FromQuadKey64(key);
      mock_server_client_->MockGetResponse(
          kLayer, child, kVersion,
          mockserver::ReadDefaultResponses::GenerateQuadTreeResponse(
              child, kQuadTreeDepth, {12}));
    }
    // quad for 11 level
    mock_server_client_->MockGetResponse(
        kLayer, base_tile, kVersion,
        mockserver::ReadDefaultResponses::GenerateQuadTreeResponse(
            base_tile, kQuadTreeDepth, {12}));

    auto levels_changed = 1;
    const olp::geo::TileKey first_child =
        base_tile.ChangedLevelBy(levels_changed);
    const std::uint64_t begin_tile_key = first_child.ToQuadKey64();
    int childCount = olp::geo::QuadKey64Helper::ChildrenAtLevel(levels_changed);
    for (std::uint64_t key = begin_tile_key; key < begin_tile_key + childCount;
         ++key) {
      auto child = olp::geo::TileKey::FromQuadKey64(key);
      const auto data_handle =
          mockserver::ReadDefaultResponses::GenerateDataHandle(
              child.ToHereTile());
      if (tiles_prefetched.size() >= 2) {
        tiles_to_protect.push_back(child);
      }
      tiles_prefetched.emplace_back(std::move(child));
      mock_server_client_->MockGetResponse(
          kLayer, data_handle,
          mockserver::ReadDefaultResponses::GenerateData());
    }
  }

  {
    SCOPED_TRACE("Prefetch tiles for levels 12 and 16");
    const auto request = olp::dataservice::read::PrefetchTilesRequest()
                             .WithTileKeys({base_tile})
                             .WithMinLevel(12)
                             .WithMaxLevel(16);
    auto future = client->PrefetchTiles(request).GetFuture();
    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    const auto result = response.MoveResult();

    EXPECT_EQ(result.size(), 4);
    for (auto tile_result : result) {
      EXPECT_SUCCESS(*tile_result);
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }
  {
    SCOPED_TRACE("Protect tiles, all on different quads");
    ASSERT_TRUE(client->Protect(tiles_to_protect));
  }
  {
    SCOPED_TRACE(
        "Prefetch tiles for levels 11 and 15, so we have different quad");
    const auto request = olp::dataservice::read::PrefetchTilesRequest()
                             .WithTileKeys({base_tile})
                             .WithMinLevel(11)
                             .WithMaxLevel(15);
    auto future = client->PrefetchTiles(request).GetFuture();
    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    const auto result = response.MoveResult();

    EXPECT_EQ(result.size(), 4);
    for (auto tile_result : result) {
      EXPECT_SUCCESS(*tile_result);
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }
  check_if_quads_protected({base_tile}, false);
  check_if_quads_protected(tiles_to_protect, true);

  ASSERT_TRUE(
      settings_->cache->Protect({"hrn:here:data::olp-here-test:hereos-internal-"
                                 "test::testlayer::5901734::44::4::quadtree"}));
  check_if_quads_protected({base_tile}, true);
  {
    SCOPED_TRACE("Release tiles");
    auto tiles_to_release = std::vector<olp::geo::TileKey>(
        tiles_prefetched.begin(), --tiles_prefetched.end());
    auto last_tile = tiles_prefetched.back();

    ASSERT_TRUE(client->Release(tiles_to_release));

    check_if_quads_protected(tiles_to_release, false);
    check_if_quads_protected({base_tile}, true);
    check_if_quads_protected({last_tile}, true);
    ASSERT_TRUE(client->Release({last_tile}));
    check_if_quads_protected({base_tile}, false);
    check_if_quads_protected({last_tile}, false);
  }
}

}  // namespace
