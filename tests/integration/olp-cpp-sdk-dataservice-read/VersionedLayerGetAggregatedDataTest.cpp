/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/utils/Dir.h>
#include <olp/dataservice/read/VersionedLayerClient.h>

#include <mocks/NetworkMock.h>

#include "matchers/NetworkUrlMatchers.h"

#include "ReadDefaultResponses.h"

namespace {

namespace read = olp::dataservice::read;
namespace client = olp::client;
namespace geo = olp::geo;
namespace http = olp::http;

using testing::_;

const auto kCatalog = "hrn:here:data::olp-here-test:catalog";
const client::HRN kCatalogHrn = client::HRN::FromString(kCatalog);
const auto kEndpoint = "https://localhost";
const auto kCachePathMutable = "./tmp_cache";

constexpr auto kWaitTimeout = std::chrono::seconds(3);

class VersionedLayerGetAggregatedDataTest : public ::testing::Test {
 public:
  void SetUp() override {
    olp::utils::Dir::remove(kCachePathMutable);

    network_mock_ = std::make_shared<NetworkMock>();

    settings_.api_lookup_settings.catalog_endpoint_provider =
        [](const client::HRN&) { return kEndpoint; };

    settings_.network_request_handler = network_mock_;
    settings_.task_scheduler =
        client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
  }

  void TearDown() override {
    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
    network_mock_.reset();
    settings_.task_scheduler.reset();
    olp::utils::Dir::remove(kCachePathMutable);
  }

  void ExpectQuadTreeRequest(const std::string& layer, int64_t version,
                             mockserver::QuadTreeBuilder quad_tree) {
    std::stringstream url;
    url << kEndpoint << "/catalogs/" << kCatalog << "/layers/" << layer
        << "/versions/" << version << "/quadkeys/"
        << quad_tree.Root().ToHereTile() << "/depths/4";

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(url.str()), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            quad_tree.BuildJson()));
  }

  void ExpectBlobRequest(const std::string& layer,
                         const std::string& data_handle,
                         const std::string& data) {
    std::stringstream url;
    url << kEndpoint << "/catalogs/" << kCatalog << "/layers/" << layer
        << "/data/" << data_handle;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(url.str()), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            data));
  }

 protected:
  client::OlpClientSettings settings_;
  std::shared_ptr<NetworkMock> network_mock_;
};

TEST_F(VersionedLayerGetAggregatedDataTest, ParentTileFarAway) {
  const auto layer = "testlayer";
  const auto ver = 7;

  const auto target_tile =
      olp::geo::TileKey::FromRowColumnLevel(6481, 8800, 14);
  const auto aggregated_parent = target_tile.ChangedLevelTo(1);

  // Mock a quad tree that bundles 0-14 levels, and a blob
  {
    const auto tree_root = target_tile.ChangedLevelTo(0);
    const auto aggregated_parent = target_tile.ChangedLevelTo(1);

    mockserver::QuadTreeBuilder tree_10(target_tile.ChangedLevelTo(10), ver);
    tree_10.WithParent(tree_root, "handle-0")
        .WithParent(aggregated_parent, "handle-1");

    mockserver::QuadTreeBuilder tree_5(target_tile.ChangedLevelTo(5), ver);
    tree_5.WithParent(tree_root, "handle-0")
        .WithParent(aggregated_parent, "handle-1");

    mockserver::QuadTreeBuilder tree_0(target_tile.ChangedLevelTo(0), ver);
    tree_0.WithSubQuad(tree_root, "handle-0")
        .WithSubQuad(aggregated_parent, "handle-1");

    ExpectQuadTreeRequest(layer, ver, tree_10);
    ExpectQuadTreeRequest(layer, ver, tree_5);
    ExpectQuadTreeRequest(layer, ver, tree_0);

    // Expect to download handle-1
    ExpectBlobRequest(layer, "handle-1", "A");
  }

  olp::cache::CacheSettings settings;
  settings.disk_path_mutable = kCachePathMutable;
  settings_.cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache(settings);

  read::VersionedLayerClient client(kCatalogHrn, layer, ver, settings_);

  auto api_call_outcome =
      client.GetAggregatedData(read::TileRequest().WithTileKey(target_tile));

  auto future = api_call_outcome.GetFuture();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);

  auto api_result = future.get();

  ASSERT_TRUE(api_result.IsSuccessful());

  const auto& aggregate_result = api_result.GetResult();

  ASSERT_EQ(aggregate_result.GetTile(), aggregated_parent);

  // Validate that all APIs can't handle it.
  ASSERT_TRUE(client.IsCached(target_tile, true));
  ASSERT_TRUE(client.IsCached(aggregated_parent));
  ASSERT_TRUE(client.Protect({aggregated_parent}));
  ASSERT_TRUE(client.Release({aggregated_parent}));
  ASSERT_TRUE(client.RemoveFromCache(aggregated_parent));

  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
}

}  // namespace
