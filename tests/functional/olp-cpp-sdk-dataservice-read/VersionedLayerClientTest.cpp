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
#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/NetworkSettings.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/dataservice/read/FetchOptions.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
#include <string>
#include <testutils/CustomParameters.hpp>
// clang-format off
#include "generated/serializer/PartitionsSerializer.h"
#include "generated/serializer/JsonSerializer.h"
// clang-format on
#include "DefaultResponses.h"
#include "MockServerHelper.h"
#include "Utils.h"

namespace {

const auto kMockServerHost = "localhost";
const auto kMockServerPort = 1080;

const auto kAppId = "id";
const auto kAppSecret = "secret";
const auto kTestHrn = "hrn:here:data::olp-here-test:hereos-internal-test";
const auto kPartitionsResponsePath =
    "/metadata/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test/"
    "layers/testlayer/partitions";

void WriteSubquadsToJson(rapidjson::Document& doc,
                         const olp::geo::TileKey& root_tile,
                         const std::vector<std::uint16_t>& sub_quads,
                         rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value sub_quads_value;
  sub_quads_value.SetArray();
  for (auto quad : sub_quads) {
    const auto partition = root_tile.AddedSubkey64(quad).ToHereTile();
    const auto data_handle =
        mockserver::DefaultResponses::GenerateDataHandle(partition);

    rapidjson::Value item_value;
    item_value.SetObject();
    olp::serializer::serialize("subQuadKey", std::to_string(quad), item_value,
                               allocator);
    olp::serializer::serialize("version", 0, item_value, allocator);
    olp::serializer::serialize("dataHandle", data_handle, item_value,
                               allocator);
    olp::serializer::serialize("dataSize", 100, item_value, allocator);
    sub_quads_value.PushBack(std::move(item_value), allocator);
  }
  doc.AddMember("subQuads", std::move(sub_quads_value), allocator);
}

void WriteParentquadsToJson(rapidjson::Document& doc,
                            const std::vector<std::uint64_t>& parent_quads,
                            rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value parent_quads_value;
  parent_quads_value.SetArray();
  for (auto parent : parent_quads) {
    const auto partition = std::to_string(parent);
    const auto data_handle =
        mockserver::DefaultResponses::GenerateDataHandle(partition);

    rapidjson::Value item_value;
    item_value.SetObject();
    olp::serializer::serialize("partition", std::to_string(parent), item_value,
                               allocator);
    olp::serializer::serialize("version", 0, item_value, allocator);
    olp::serializer::serialize("dataHandle", data_handle, item_value,
                               allocator);
    olp::serializer::serialize("dataSize", 100, item_value, allocator);
    parent_quads_value.PushBack(std::move(item_value), allocator);
  }
  doc.AddMember("parentQuads", std::move(parent_quads_value), allocator);
}

std::string GenerateQuadTreeResponse(
    olp::geo::TileKey root_tile, std::uint32_t depth,
    const std::vector<std::uint32_t>& available_levels) {
  std::vector<std::uint16_t> sub_quads;
  std::vector<std::uint64_t> parent_quads;

  // generate data
  for (auto level : available_levels) {
    if (level < root_tile.Level()) {
      auto key = root_tile.ChangedLevelTo(level);
      parent_quads.push_back(key.ToQuadKey64());
    } else {
      const auto level_depth = level - root_tile.Level();
      if (level_depth > depth) {
        continue;
      }

      const auto sub_tile =
          olp::geo::TileKey::FromRowColumnLevel(0, 0, level_depth);
      const auto start_level_id = sub_tile.ToQuadKey64();
      const auto tiles_count =
          olp::geo::QuadKey64Helper::ChildrenAtLevel(level_depth);

      std::vector<std::uint64_t> layer_ids(tiles_count);
      std::iota(layer_ids.begin(), layer_ids.end(), start_level_id);
      sub_quads.insert(sub_quads.end(), layer_ids.begin(), layer_ids.end());
    }
  }

  rapidjson::Document doc;
  auto& allocator = doc.GetAllocator();
  doc.SetObject();
  WriteSubquadsToJson(doc, root_tile, sub_quads, allocator);
  WriteParentquadsToJson(doc, parent_quads, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

class VersionedLayerClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();

    olp::authentication::Settings auth_settings({kAppId, kAppSecret});
    auth_settings.network_request_handler = network;

    // setup proxy
    auth_settings.network_proxy_settings =
        olp::http::NetworkProxySettings()
            .WithHostname(kMockServerHost)
            .WithPort(kMockServerPort)
            .WithType(olp::http::NetworkProxySettings::Type::HTTP);
    olp::authentication::TokenProviderDefault provider(auth_settings);
    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;

    settings_ = std::make_shared<olp::client::OlpClientSettings>();
    settings_->network_request_handler = network;
    settings_->authentication_settings = auth_client_settings;

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

TEST_F(VersionedLayerClientTest, GetPartitions) {
  olp::client::HRN hrn(kTestHrn);
  {
    mock_server_client_->MockAuth();
    mock_server_client_->MockLookupResourceApiResponse(
        mockserver::DefaultResponses::GenerateResourceApisResponse(kTestHrn));
    mock_server_client_->MockGetVersionResponse(
        mockserver::DefaultResponses::GenerateVersionResponse(44));
    mock_server_client_->MockGetResponse(
        mockserver::DefaultResponses::GeneratePartitionsResponse(4),
        kPartitionsResponsePath);
  }

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto future = catalog_client->GetPartitions(request);
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
  const auto request = olp::dataservice::read::TileRequest().WithTileKey(tile);

  // authentification not needed for the test
  settings_->authentication_settings = boost::none;

  {
    SCOPED_TRACE("Requested tile");
    const auto data_handle =
        mockserver::DefaultResponses::GenerateDataHandle(tile.ToHereTile());
    const auto data = mockserver::DefaultResponses::GenerateData();

    {
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::DefaultResponses::GenerateResourceApisResponse(kTestHrn));
      mock_server_client_->MockGetVersionResponse(
          mockserver::DefaultResponses::GenerateVersionResponse(kVersion));
      mock_server_client_->MockGetResponse(
          kLayer, root_tile, kVersion,
          GenerateQuadTreeResponse(root_tile, kQuadTreeDepth,
                                   {1, 3, 12, 13, 14, 15}));
      mock_server_client_->MockGetResponse(kLayer, data_handle, data);
    }

    auto client =
        std::make_unique<olp::dataservice::read::VersionedLayerClient>(
            hrn, kLayer, boost::none, *settings_);

    auto future = client->GetAggregatedData(request).GetFuture();
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
    const auto data_handle = mockserver::DefaultResponses::GenerateDataHandle(
        expect_tile.ToHereTile());
    const auto data = mockserver::DefaultResponses::GenerateData();

    {
      SetUpMockServer(settings_->network_request_handler);
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::DefaultResponses::GenerateResourceApisResponse(kTestHrn));
      mock_server_client_->MockGetVersionResponse(
          mockserver::DefaultResponses::GenerateVersionResponse(kVersion));
      mock_server_client_->MockGetResponse(
          kLayer, root_tile, kVersion,
          GenerateQuadTreeResponse(root_tile, kQuadTreeDepth,
                                   {1, 3, 12, 13, 14}));
      mock_server_client_->MockGetResponse(kLayer, data_handle, data);
    }

    auto client =
        std::make_unique<olp::dataservice::read::VersionedLayerClient>(
            hrn, kLayer, boost::none, *settings_);

    auto future = client->GetAggregatedData(request).GetFuture();
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
    const auto data_handle = mockserver::DefaultResponses::GenerateDataHandle(
        expect_tile.ToHereTile());
    const auto data = mockserver::DefaultResponses::GenerateData();

    {
      SetUpMockServer(settings_->network_request_handler);
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::DefaultResponses::GenerateResourceApisResponse(kTestHrn));
      mock_server_client_->MockGetVersionResponse(
          mockserver::DefaultResponses::GenerateVersionResponse(kVersion));
      mock_server_client_->MockGetResponse(
          kLayer, root_tile, kVersion,
          GenerateQuadTreeResponse(root_tile, kQuadTreeDepth, {1, 2, 3}));
      mock_server_client_->MockGetResponse(kLayer, data_handle, data);
    }

    auto client =
        std::make_unique<olp::dataservice::read::VersionedLayerClient>(
            hrn, kLayer, boost::none, *settings_);

    auto future = client->GetAggregatedData(request).GetFuture();
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
        mockserver::DefaultResponses::GenerateDataHandle(tile.ToHereTile());
    const auto data = mockserver::DefaultResponses::GenerateData();

    {
      SetUpMockServer(settings_->network_request_handler);
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::DefaultResponses::GenerateResourceApisResponse(kTestHrn));
      mock_server_client_->MockGetVersionResponse(
          mockserver::DefaultResponses::GenerateVersionResponse(kVersion));
      mock_server_client_->MockGetResponse(
          kLayer, root_tile, kVersion,
          GenerateQuadTreeResponse(root_tile, kQuadTreeDepth, {15}));
      mock_server_client_->MockGetResponse(kLayer, data_handle, data);
    }

    settings_->cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    auto client =
        std::make_unique<olp::dataservice::read::VersionedLayerClient>(
            hrn, kLayer, boost::none, *settings_);

    auto future = client->GetAggregatedData(request).GetFuture();
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
        olp::dataservice::read::TileRequest().WithTileKey(tile).WithFetchOption(
            olp::dataservice::read::CacheOnly);
    response = client->GetAggregatedData(request).GetFuture().get();
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
