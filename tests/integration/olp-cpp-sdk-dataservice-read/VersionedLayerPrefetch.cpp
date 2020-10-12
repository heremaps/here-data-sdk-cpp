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

#include <olp/dataservice/read/PrefetchTileResult.h>
#include <olp/dataservice/read/PrefetchTilesRequest.h>

namespace {

namespace read = olp::dataservice::read;
namespace http = olp::http;
using olp::http::HttpStatusCode;

constexpr auto kWaitTimeout = std::chrono::seconds(3);

class VersionedLayerPrefetch : public VersionedLayerTestBase {};

TEST_F(VersionedLayerPrefetch, AggregatedPrefetch) {
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
      client.PrefetchTiles(read::PrefetchTilesRequest()
                               .WithTileKeys({target_tile})
                               .WithDataAggregationEnabled(true));

  auto future = api_call_outcome.GetFuture();

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);

  auto result = future.get();

  ASSERT_TRUE(result.IsSuccessful());

  const auto& prefetch_result = result.GetResult();

  ASSERT_TRUE(!prefetch_result.empty());

  const auto& prefetched_tile = prefetch_result.front();

  ASSERT_TRUE(prefetched_tile->IsSuccessful());

  ASSERT_EQ(prefetched_tile->tile_key_, aggregated_parent);

  // Validate that all APIs can handle it.
  ASSERT_TRUE(client.IsCached(target_tile, true));
  ASSERT_TRUE(client.IsCached(aggregated_parent));
  ASSERT_TRUE(client.Protect({aggregated_parent}));
  ASSERT_TRUE(client.Release({aggregated_parent}));
  ASSERT_TRUE(client.RemoveFromCache(aggregated_parent));
}

TEST_F(VersionedLayerPrefetch, SomeQueryFailsWithoutLevels) {
  const auto layer_version = 7;
  const auto target_tile =
      olp::geo::TileKey::FromRowColumnLevel(6481, 8800, 14);
  const auto target_tile2 = olp::geo::TileKey::FromRowColumnLevel(10, 10, 9);

  // Mock a quad trees and a blobs
  {
    const auto tree_root = target_tile.ChangedLevelTo(0);
    mockserver::QuadTreeBuilder tree_10(target_tile.ChangedLevelTo(10),
                                        layer_version);
    tree_10.WithParent(tree_root, "handle-0")
        .WithSubQuad(target_tile, "handle-1");

    mockserver::QuadTreeBuilder tree_5(target_tile2.ChangedLevelTo(5),
                                       layer_version);
    tree_5.WithParent(tree_root, "handle-0")
        .WithSubQuad(target_tile2, "handle-2");

    ExpectQuadTreeRequest(layer_version, tree_10);
    ExpectQuadTreeRequest(
        layer_version, tree_5,
        http::NetworkResponse().WithStatus(HttpStatusCode::BAD_REQUEST));
    // Expect to download handle-1
    ExpectBlobRequest("handle-1", "A");
  }

  read::VersionedLayerClient client(kCatalogHrn, kLayerName, layer_version,
                                    settings_);
  auto api_call_outcome = client.PrefetchTiles(
      read::PrefetchTilesRequest().WithTileKeys({target_tile, target_tile2}));

  auto future = api_call_outcome.GetFuture();
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  auto result = future.get();
  ASSERT_TRUE(result.IsSuccessful());
  const auto& prefetch_result = result.GetResult();
  ASSERT_TRUE(!prefetch_result.empty());
  for (auto& res : prefetch_result) {
    if (res->tile_key_ == target_tile) {
      ASSERT_TRUE(res->IsSuccessful());
    }
    if (res->tile_key_ == target_tile2) {
      ASSERT_FALSE(res->IsSuccessful());
      ASSERT_EQ(res->GetError().GetErrorCode(),
                olp::client::ErrorCode::NotFound);
    }
  }
  // Validate that all APIs can handle it.
  ASSERT_TRUE(client.IsCached(target_tile, true));
}

TEST_F(VersionedLayerPrefetch, SomeQueryFailsWithLevels) {
  const auto layer_version = 7;
  const auto target_tile =
      olp::geo::TileKey::FromRowColumnLevel(6481, 8800, 14);
  const std::vector<olp::geo::TileKey> children = {
      target_tile.GetChild(0), target_tile.GetChild(1), target_tile.GetChild(2),
      target_tile.GetChild(3)};

  // Mock a quad trees and a blobs
  {
    const auto tree_root = target_tile.ChangedLevelTo(0);

    for (auto i = 0u; i < children.size(); i++) {
      const auto& child = children.at(i);
      auto handle = "handle-" + std::to_string(i);
      mockserver::QuadTreeBuilder tree(child, layer_version);
      tree.WithParent(tree_root, "handle-0").WithSubQuad(child, handle);
      if (i % 2 == 0) {
        ExpectQuadTreeRequest(layer_version, tree);
        ExpectBlobRequest(handle, "A");
      } else {
        ExpectQuadTreeRequest(
            layer_version, tree,
            http::NetworkResponse().WithStatus(HttpStatusCode::BAD_REQUEST));
      }
    }
  }

  read::VersionedLayerClient client(kCatalogHrn, kLayerName, layer_version,
                                    settings_);
  auto api_call_outcome = client.PrefetchTiles(read::PrefetchTilesRequest()
                                                   .WithTileKeys({target_tile})
                                                   .WithMinLevel(15)
                                                   .WithMaxLevel(19));
  auto future = api_call_outcome.GetFuture();
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);

  auto result = future.get();
  ASSERT_TRUE(result.IsSuccessful());
  const auto& prefetch_result = result.GetResult();
  ASSERT_TRUE(!prefetch_result.empty());
  ASSERT_EQ(prefetch_result.size(), 2);

  for (auto& res : prefetch_result) {
    ASSERT_TRUE(res->IsSuccessful());
  }
}

}  // namespace
