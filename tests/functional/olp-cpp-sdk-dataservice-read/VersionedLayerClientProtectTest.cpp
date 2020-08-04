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
#include "ApiDefaultResponses.h"
#include "MockServerHelper.h"
#include "ReadDefaultResponses.h"
#include "Utils.h"

namespace {

const auto kMockServerHost = "localhost";
const auto kMockServerPort = 1080;
const auto kTestHrn = "hrn:here:data::olp-here-test:hereos-internal-test";

class VersionedLayerClientProtectTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();

    settings_ = std::make_shared<olp::client::OlpClientSettings>();
    olp::cache::CacheSettings cache_settings;
    const std::string cache_path = olp::utils::Dir::TempDirectory() + "/test";
    cache_settings.disk_path_mutable = cache_path;
    olp::utils::Dir::remove(cache_path);
    cache_settings.max_memory_cache_size = 0u;
    cache_settings.eviction_policy =
        olp::cache::EvictionPolicy::kLeastRecentlyUsed;
    cache_settings.max_disk_storage =
        820052u / 0.85;  // eviction trashold is (0.8500000238F)
    settings_->cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache(
            cache_settings);
    settings_->network_request_handler = network;
    // setup proxy
    settings_->proxy_settings =
        olp::http::NetworkProxySettings()
            .WithHostname(kMockServerHost)
            .WithPort(kMockServerPort)
            .WithType(olp::http::NetworkProxySettings::Type::HTTP);
    SetUpMockServer(network);
  }

  void TearDown() override {
    auto network = std::move(settings_->network_request_handler);
    settings_.reset();
    mock_server_client_.reset();
  }

  void SetUpMockServer(std::shared_ptr<olp::http::Network> network) {
    // create client to set mock server expectations
    olp::client::OlpClientSettings olp_client_settings;
    olp_client_settings.network_request_handler = network;
    mock_server_client_ = std::make_shared<mockserver::MockServerHelper>(
        olp_client_settings, kTestHrn);
  }

  std::shared_ptr<olp::client::OlpClientSettings> settings_;
  std::shared_ptr<mockserver::MockServerHelper> mock_server_client_;
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
  tiles_lover_levels.reserve(3840);
  tiles_upper_levels.reserve(240);

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
                child, kQuadTreeDepth, {13, 14, 15, 16}));
      }

      auto levels_changed = 2;
      while (levels_changed < 6) {
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
          if (child.Level() < 15) {
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
    SCOPED_TRACE("Prefetch tiles for levels 15 and 16");
    const auto request = olp::dataservice::read::PrefetchTilesRequest()
                             .WithTileKeys(request_tiles)
                             .WithMinLevel(15)
                             .WithMaxLevel(16);
    auto future = client->PrefetchTiles(request).GetFuture();
    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    const auto result = response.MoveResult();

    EXPECT_EQ(result.size(), 3840);
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
}  // namespace
