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

#include <gtest/gtest.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>

#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
#include "repositories/QuadTreeIndex.h"

namespace {
namespace read = olp::dataservice::read;
namespace model = olp::dataservice::read::model;
using ::testing::_;

const std::string kCatalog =
    "hrn:here:data::olp-here-test:hereos-internal-test-v2";
const std::string kLayerId = "testlayer";
const auto kHrn = olp::client::HRN::FromString(kCatalog);
const auto kPartitionId = "269";
const auto kCatalogVersion = 108;
const auto kTimeout = std::chrono::seconds(5);
constexpr auto kBlobDataHandle = R"(4eed6ed1-0d32-43b9-ae79-043cb4256432)";
constexpr auto kHereTile = "23618364";
constexpr auto kHereTileDataHandle = "BD53A6D60A34C20DC42ACAB2650FE361.48";
constexpr auto kQuadkeyResponse =
    R"jsonString({"subQuads": [{"subQuadKey": "4","version":282,"dataHandle":"7636348E50215979A39B5F3A429EDDB4.282","dataSize":277},{"subQuadKey":"5","version":282,"dataHandle":"8C9B3E08E294ADB2CD07EBC8412062FE.282","dataSize":271},{"subQuadKey": "6","version":282,"dataHandle":"9772F5E1822DFF25F48F150294B1ECF5.282","dataSize":289},{"subQuadKey":"7","version":282,"dataHandle":"BF84D8EC8124B96DBE5C4DB68B05918F.282","dataSize":283},{"subQuadKey":"1","version":48,"dataHandle":"BD53A6D60A34C20DC42ACAB2650FE361.48","dataSize":89}],"parentQuads":[{"partition":"23","version":282,"dataHandle":"F8F4C3CB09FBA61B927256CBCB8441D1.282","dataSize":52438},{"partition":"5","version":282,"dataHandle":"13E2C624E0136C3357D092EE7F231E87.282","dataSize":99151},{"partition":"95","version":253,"dataHandle":"B6F7614316BB8B81478ED7AE370B22A6.253","dataSize":6765}]})jsonString";

TEST(VersionedLayerClientTest, CanBeMoved) {
  read::VersionedLayerClient client_a(olp::client::HRN(), "", boost::none, {});
  read::VersionedLayerClient client_b(std::move(client_a));
  read::VersionedLayerClient client_c(olp::client::HRN(), "", boost::none, {});
  client_c = std::move(client_b);
}

TEST(VersionedLayerClientTest, GetData) {
  std::shared_ptr<NetworkMock> network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();
  olp::client::OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;

  read::VersionedLayerClient client(kHrn, kLayerId, boost::none, settings);
  {
    SCOPED_TRACE("Get Data with PartitionId and DataHandle");
    std::promise<read::DataResponse> promise;
    std::future<read::DataResponse> future = promise.get_future();

    auto token = client.GetData(
        read::DataRequest()
            .WithPartitionId(kPartitionId)
            .WithDataHandle(kBlobDataHandle),
        [&](read::DataResponse response) { promise.set_value(response); });

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::PreconditionFailed);
  }
}

TEST(VersionedLayerClientTest, RemoveFromCachePartition) {
  olp::client::OlpClientSettings settings;
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();
  settings.cache = cache_mock;

  // successfull mock cache calls
  auto found_cache_response = [](const std::string& /*key*/,
                                 const olp::cache::Decoder& /*encoder*/) {
    auto partition = model::Partition();
    partition.SetPartition(kPartitionId);
    partition.SetDataHandle(kBlobDataHandle);
    return partition;
  };
  auto partition_cache_remove = [&](const std::string& prefix) {
    std::string expected_prefix =
        kHrn.ToCatalogHRNString() + "::" + kLayerId + "::" + kPartitionId +
        "::" + std::to_string(kCatalogVersion) + "::partition";
    EXPECT_EQ(prefix, expected_prefix);
    return true;
  };
  auto data_cache_remove = [&](const std::string& prefix) {
    std::string expected_prefix = kHrn.ToCatalogHRNString() + "::" + kLayerId +
                                  "::" + kBlobDataHandle + "::Data";
    EXPECT_EQ(prefix, expected_prefix);
    return true;
  };

  read::VersionedLayerClient client(kHrn, kLayerId, kCatalogVersion, settings);
  {
    SCOPED_TRACE("Successfull remove partition from cache");

    EXPECT_CALL(*cache_mock, Get(_, _)).WillOnce(found_cache_response);
    EXPECT_CALL(*cache_mock, RemoveKeysWithPrefix(_))
        .WillOnce(partition_cache_remove)
        .WillOnce(data_cache_remove);
    ASSERT_TRUE(client.RemoveFromCache(kPartitionId));
  }
  {
    SCOPED_TRACE("Remove not existing partition from cache");
    EXPECT_CALL(*cache_mock, Get(_, _))
        .WillOnce([](const std::string&, const olp::cache::Decoder&) {
          return boost::any();
        });
    ASSERT_TRUE(client.RemoveFromCache(kPartitionId));
  }
  {
    SCOPED_TRACE("Partition cache failure");
    EXPECT_CALL(*cache_mock, Get(_, _)).WillOnce(found_cache_response);
    EXPECT_CALL(*cache_mock, RemoveKeysWithPrefix(_))
        .WillOnce([](const std::string&) { return false; });
    ASSERT_FALSE(client.RemoveFromCache(kPartitionId));
  }
  {
    SCOPED_TRACE("Data cache failure");
    EXPECT_CALL(*cache_mock, Get(_, _)).WillOnce(found_cache_response);
    EXPECT_CALL(*cache_mock, RemoveKeysWithPrefix(_))
        .WillOnce(partition_cache_remove)
        .WillOnce([](const std::string&) { return false; });
    ASSERT_FALSE(client.RemoveFromCache(kPartitionId));
  }
}

TEST(VersionedLayerClientTest, RemoveFromCacheTileKey) {
  olp::client::OlpClientSettings settings;
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();
  settings.cache = cache_mock;

  auto depth = 4;
  auto tile_key = olp::geo::TileKey::FromHereTile(kHereTile);
  auto stream = std::stringstream(kQuadkeyResponse);
  read::QuadTreeIndex quad_tree(tile_key, depth, stream);
  auto buffer = quad_tree.GetRawData();

  auto root = tile_key.ChangedLevelBy(-depth);

  auto quad_cache_key = [&depth](const olp::geo::TileKey& key) {
    return kHrn.ToCatalogHRNString() + "::" + kLayerId +
           "::" + key.ToHereTile() + "::" + std::to_string(kCatalogVersion) +
           "::" + std::to_string(depth) + "::quadtree";
  };

  // successfull mock cache calls
  auto found_cache_response = [&buffer, &root,
                               &quad_cache_key](const std::string& key) {
    EXPECT_EQ(key, quad_cache_key(root));
    return buffer;
  };
  auto data_cache_remove = [&](const std::string& prefix) {
    std::string expected_prefix = kHrn.ToCatalogHRNString() + "::" + kLayerId +
                                  "::" + kHereTileDataHandle + "::Data";
    EXPECT_EQ(prefix, expected_prefix);
    return true;
  };

  read::VersionedLayerClient client(kHrn, kLayerId, kCatalogVersion, settings);
  {
    SCOPED_TRACE("Successfull remove tile from cache");

    EXPECT_CALL(*cache_mock, Get(_)).WillOnce(found_cache_response);
    EXPECT_CALL(*cache_mock, RemoveKeysWithPrefix(_))
        .WillOnce(data_cache_remove);
    ASSERT_TRUE(client.RemoveFromCache(tile_key));
  }
  {
    SCOPED_TRACE("Remove not existing tile from cache");
    EXPECT_CALL(*cache_mock, Get(_))
        .WillOnce([&tile_key, &quad_cache_key](const std::string& key) {
          EXPECT_EQ(key, quad_cache_key(tile_key.ChangedLevelBy(-4)));
          return nullptr;
        })
        .WillOnce([&tile_key, &quad_cache_key](const std::string& key) {
          EXPECT_EQ(key, quad_cache_key(tile_key.ChangedLevelBy(-3)));
          return nullptr;
        })
        .WillOnce([&tile_key, &quad_cache_key](const std::string& key) {
          EXPECT_EQ(key, quad_cache_key(tile_key.ChangedLevelBy(-2)));
          return nullptr;
        })
        .WillOnce([&tile_key, &quad_cache_key](const std::string& key) {
          EXPECT_EQ(key, quad_cache_key(tile_key.ChangedLevelBy(-1)));
          return nullptr;
        })
        .WillOnce([&tile_key, &quad_cache_key](const std::string& key) {
          EXPECT_EQ(key, quad_cache_key(tile_key));
          return nullptr;
        });
    ASSERT_TRUE(client.RemoveFromCache(tile_key));
  }
  {
    SCOPED_TRACE("Data cache failure");
    EXPECT_CALL(*cache_mock, Get(_)).WillOnce(found_cache_response);
    EXPECT_CALL(*cache_mock, RemoveKeysWithPrefix(_))
        .WillOnce([](const std::string&) { return false; });
    ASSERT_FALSE(client.RemoveFromCache(tile_key));
  }
}

}  // namespace
