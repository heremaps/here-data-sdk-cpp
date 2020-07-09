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
#include <olp/core/http/NetworkSettings.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/dataservice/read/FetchOptions.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
#include <string>
#include <testutils/CustomParameters.hpp>
#include "ApiDefaultResponses.h"
#include "MockServerHelper.h"
#include "ReadDefaultResponses.h"
#include "Utils.h"

namespace {

const auto kMockServerHost = "localhost";
const auto kMockServerPort = 1080;
const auto kTestHrn = "hrn:here:data::olp-here-test:hereos-internal-test";

class VersionedLayerClientPrefetchTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();

    settings_ = std::make_shared<olp::client::OlpClientSettings>();
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

TEST_F(VersionedLayerClientPrefetchTest, Prefetch) {
  olp::client::HRN hrn(kTestHrn);

  constexpr auto kTileId = "5901734";
  constexpr auto kLayer = "testlayer";
  constexpr auto kQuadTreeDepth = 4;
  constexpr auto kVersion = 44;

  const auto root_tile = olp::geo::TileKey::FromHereTile(kTileId);
  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      hrn, kLayer, boost::none, *settings_);
  std::vector<std::string> tiles_data;
  tiles_data.reserve(4);

  {
    SCOPED_TRACE("Prefetch tiles");
    const auto tile = root_tile.ChangedLevelTo(12);
    const auto request = olp::dataservice::read::PrefetchTilesRequest()
                             .WithTileKeys({tile})
                             .WithMinLevel(12)
                             .WithMaxLevel(15);
    {
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

    auto future = client->PrefetchTiles(request).GetFuture();
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
              ->GetData(olp::dataservice::read::TileRequest()
                            .WithTileKey(child)
                            .WithFetchOption(olp::dataservice::read::CacheOnly))
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
}
}  // namespace
