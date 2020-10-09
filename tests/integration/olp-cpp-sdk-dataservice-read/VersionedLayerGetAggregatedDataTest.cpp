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

#include "VersionedLayerTestBase.h"

namespace {

namespace read = olp::dataservice::read;

constexpr auto kWaitTimeout = std::chrono::seconds(3);

class VersionedLayerGetAggregatedDataTest : public VersionedLayerTestBase {};

TEST_F(VersionedLayerGetAggregatedDataTest, ParentTileFarAway) {
  const auto layer_version = 7;

  const auto target_tile =
      olp::geo::TileKey::FromRowColumnLevel(6481, 8800, 14);
  const auto aggregated_parent = target_tile.ChangedLevelTo(1);

  // Mock a quad tree that bundles 0-14 levels, and a blob
  {
    const auto tree_root = target_tile.ChangedLevelTo(0);
    const auto aggregated_parent = target_tile.ChangedLevelTo(1);

    mockserver::QuadTreeBuilder tree_10(target_tile.ChangedLevelTo(10),
                                        layer_version);
    tree_10.WithParent(tree_root, "handle-0")
        .WithParent(aggregated_parent, "handle-1");

    mockserver::QuadTreeBuilder tree_5(target_tile.ChangedLevelTo(5),
                                       layer_version);
    tree_5.WithParent(tree_root, "handle-0")
        .WithParent(aggregated_parent, "handle-1");

    mockserver::QuadTreeBuilder tree_0(target_tile.ChangedLevelTo(0),
                                       layer_version);
    tree_0.WithSubQuad(tree_root, "handle-0")
        .WithSubQuad(aggregated_parent, "handle-1");

    ExpectQuadTreeRequest(layer_version, tree_10);
    ExpectQuadTreeRequest(layer_version, tree_5);
    ExpectQuadTreeRequest(layer_version, tree_0);

    // Expect to download handle-1
    ExpectBlobRequest("handle-1", "A");
  }

  read::VersionedLayerClient client(kCatalogHrn, kLayerName, layer_version,
                                    settings_);

  auto api_call_outcome =
      client.GetAggregatedData(read::TileRequest().WithTileKey(target_tile));

  auto future = api_call_outcome.GetFuture();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);

  auto api_result = future.get();

  ASSERT_TRUE(api_result.IsSuccessful());

  const auto& aggregate_result = api_result.GetResult();

  ASSERT_EQ(aggregate_result.GetTile(), aggregated_parent);

  // Validate that all APIs can handle it.
  ASSERT_TRUE(client.IsCached(target_tile, true));
  ASSERT_TRUE(client.IsCached(aggregated_parent));
  ASSERT_TRUE(client.Protect({aggregated_parent}));
  ASSERT_TRUE(client.Release({aggregated_parent}));
  ASSERT_TRUE(client.RemoveFromCache(aggregated_parent));

  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
}

}  // namespace
