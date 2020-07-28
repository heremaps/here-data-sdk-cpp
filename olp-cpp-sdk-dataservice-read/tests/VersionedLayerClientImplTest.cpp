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
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>

#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/utils/Dir.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
#include "VersionedLayerClientImpl.h"
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
constexpr auto kOtherHereTile = "1476147";
constexpr auto kOtherHereTile2 = "5904591";
constexpr auto kHereTileDataHandle = "f9a9fd8e-eb1b-48e5-bfdb-4392b3826443";
constexpr auto kUrlQuadRequest =
    R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/versions/4/quadkeys/92259/depths/4)";
constexpr auto kHttpResponceQuadkey =
    R"jsonString({"subQuads": [{"subQuadKey":"19","version":4,"dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1"},{"subQuadKey":"316","version":4,"dataHandle":"f9a9fd8e-eb1b-48e5-bfdb-4392b3826443"},{"subQuadKey":"317","version":4,"dataHandle":"e119d20e-c7c6-4563-ae88-8aa5c6ca75c3"},{"subQuadKey":"318","version":4,"dataHandle":"a7a1afdf-db7e-4833-9627-d38bee6e2f81"},{"subQuadKey":"319","version":4,"dataHandle":"9d515348-afce-44e8-bc6f-3693cfbed104"},{"subQuadKey":"79","version":4,"dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1"}],"parentQuads": []})jsonString";
constexpr auto kUrlLookup =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data::olp-here-test:hereos-internal-test-v2/apis)";
constexpr auto kHttpResponseLookup =
    R"jsonString([{"api":"metadata","version":"v1","baseURL":"https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2","parameters":{}}, {"api":"query","version":"v1","baseURL":"https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2","parameters":{}}, {"api":"blob","version":"v1","baseURL":"https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2","parameters":{}},{"api":"volatile-blob","version":"v1","baseURL":"https://volatile-blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2","parameters":{}},{"api":"stream","version":"v2","baseURL":"https://stream-ireland.data.api.platform.here.com/stream/v2/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";
constexpr auto kUrlVersion =
    R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/versions/latest?startVersion=-1)";
constexpr auto kHttpResponseVersion =
    R"jsonString({"version":4})jsonString";
constexpr auto kDataRequestTile =
    R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/f9a9fd8e-eb1b-48e5-bfdb-4392b3826443)";
constexpr auto kDataRequestOtherTile =
    R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/95c5c703-e00e-4c38-841e-e419367474f1)";
constexpr auto kDataRequestOtherTile2 =
    R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1)";
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
  auto root = tile_key.ChangedLevelBy(-depth);
  auto stream = std::stringstream(kHttpResponceQuadkey);
  read::QuadTreeIndex quad_tree(root, depth, stream);
  auto buffer = quad_tree.GetRawData();

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
    EXPECT_CALL(*cache_mock, Contains(_))
        .WillRepeatedly([&](const std::string&) { return true; });
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
  {
    SCOPED_TRACE("Successfull remove tile and quad tree from cache");
    EXPECT_CALL(*cache_mock, Get(_)).WillOnce(found_cache_response);
    EXPECT_CALL(*cache_mock, RemoveKeysWithPrefix(_))
        .WillOnce(data_cache_remove)
        .WillOnce([&](const std::string&) { return true; });
    EXPECT_CALL(*cache_mock, Contains(_))
        .WillRepeatedly([&](const std::string&) { return false; });
    ASSERT_TRUE(client.RemoveFromCache(tile_key));
  }
  {
    SCOPED_TRACE("Successfull remove tile but removing quad tree fails");
    EXPECT_CALL(*cache_mock, Get(_)).WillOnce(found_cache_response);
    EXPECT_CALL(*cache_mock, RemoveKeysWithPrefix(_))
        .WillOnce(data_cache_remove)
        .WillOnce([&](const std::string&) { return false; });
    EXPECT_CALL(*cache_mock, Contains(_))
        .WillRepeatedly([&](const std::string&) { return false; });
    ASSERT_FALSE(client.RemoveFromCache(tile_key));
  }
}

TEST(VersionedLayerClientTest, ProtectThanRelease) {
  std::shared_ptr<NetworkMock> network_mock = std::make_shared<NetworkMock>();
  olp::cache::CacheSettings cache_settings;
  cache_settings.disk_path_mutable =
      olp::utils::Dir::TempDirectory() + "/unittest";
  auto cache =
      std::make_shared<olp::cache::DefaultCache>(std::move(cache_settings));
  cache->Open();
  cache->Clear();
  olp::client::OlpClientSettings settings;
  settings.cache = cache;
  settings.default_cache_expiration = std::chrono::seconds(2);
  settings.network_request_handler = network_mock;

  read::VersionedLayerClientImpl client(kHrn, kLayerId, boost::none, settings);
  {
    SCOPED_TRACE("Cache tile key");
    auto tile_key = olp::geo::TileKey::FromHereTile(kHereTile);

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlLookup), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kHttpResponseLookup));
    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlVersion), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kHttpResponseVersion));
    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlQuadRequest), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kHttpResponceQuadkey));

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kDataRequestTile), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     "data"));

    auto future =
        client.GetData(read::TileRequest().WithTileKey(tile_key)).GetFuture();

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());
  }
  {
    SCOPED_TRACE("Cache tile other key");
    auto tile_key = olp::geo::TileKey::FromHereTile(kOtherHereTile);

    EXPECT_CALL(*network_mock,
                Send(IsGetRequest(kDataRequestOtherTile), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     "data"));

    auto future =
        client.GetData(read::TileRequest().WithTileKey(tile_key)).GetFuture();

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());
  }
  {
    SCOPED_TRACE("Protect");
    auto tile_key = olp::geo::TileKey::FromHereTile(kHereTile);
    auto other_tile_key = olp::geo::TileKey::FromHereTile(kOtherHereTile);
    auto response = client.Protect({tile_key, other_tile_key});
    ASSERT_TRUE(response);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_TRUE(client.IsCached(tile_key));
    ASSERT_TRUE(client.IsCached(other_tile_key));
  }
  {
    SCOPED_TRACE("Protect tile which not in cache but has known data handle");
    auto tile_key = olp::geo::TileKey::FromHereTile(kOtherHereTile2);
    auto response = client.Protect({tile_key});
    ASSERT_TRUE(response);
    ASSERT_FALSE(client.IsCached(tile_key));

    // now get protected tile
    EXPECT_CALL(*network_mock,
                Send(IsGetRequest(kDataRequestOtherTile2), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     "data"));

    auto data_future =
        client.GetData(read::TileRequest().WithTileKey(tile_key)).GetFuture();

    const auto& data_response = data_future.get();
    ASSERT_TRUE(data_response.IsSuccessful());
    std::this_thread::sleep_for(std::chrono::seconds(3));
    // tile stays in cache, as it was protected before
    ASSERT_TRUE(client.IsCached(tile_key));
  }
  {
    SCOPED_TRACE("Protect tile which not in cache");
    auto tile_key = olp::geo::TileKey::FromHereTile("5904592");

    auto response = client.Protect({tile_key});
    ASSERT_FALSE(response);
  }
  {
    SCOPED_TRACE("Release tiles without releasing quad tree");
    auto tile_key = olp::geo::TileKey::FromHereTile(kHereTile);
    auto other_tile_key = olp::geo::TileKey::FromHereTile(kOtherHereTile);
    auto other_tile_key2 = olp::geo::TileKey::FromHereTile(kOtherHereTile2);
    auto response = client.Release({tile_key, other_tile_key2});
    ASSERT_TRUE(response);
    ASSERT_FALSE(client.IsCached(tile_key));
    // other_tile_key is still protected, quad tree should ce in cache
    ASSERT_TRUE(client.IsCached(other_tile_key));
  }
  {
    SCOPED_TRACE("Release last protected tile with quad tree");
    auto other_tile_key = olp::geo::TileKey::FromHereTile(kOtherHereTile);
    // release last protected tile for quad
    // 2 keys should be released(tile and quad)
    auto response = client.Release({other_tile_key});
    ASSERT_TRUE(response);
    ASSERT_FALSE(client.IsCached(other_tile_key));
  }
  {
    SCOPED_TRACE("Release not protected tile");
    auto other_tile_key = olp::geo::TileKey::FromHereTile(kOtherHereTile);
    // release last protected tile for quad
    // 2 keys should be released(tile and quad)
    auto response = client.Release({other_tile_key});
    ASSERT_FALSE(response);
  }
  {
    SCOPED_TRACE("Protect and release keys within one quad");
    auto tile_key = olp::geo::TileKey::FromHereTile(kHereTile);
    auto other_tile_key = olp::geo::TileKey::FromHereTile(kOtherHereTile);

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlQuadRequest), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kHttpResponceQuadkey));
    EXPECT_CALL(*network_mock, Send(IsGetRequest(kDataRequestTile), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     "data"));
    EXPECT_CALL(*network_mock,
                Send(IsGetRequest(kDataRequestOtherTile), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     "data"));
    auto future =
        client.GetData(read::TileRequest().WithTileKey(tile_key)).GetFuture();

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());
    future = client.GetData(read::TileRequest().WithTileKey(other_tile_key))
                 .GetFuture();
    const auto& response_other = future.get();
    ASSERT_TRUE(response_other.IsSuccessful());

    auto protect_response = client.Protect({tile_key, other_tile_key});
    ASSERT_TRUE(protect_response);
    ASSERT_TRUE(client.IsCached(tile_key));
    ASSERT_TRUE(client.IsCached(other_tile_key));

    auto release_response = client.Release({tile_key, other_tile_key});
    ASSERT_TRUE(release_response);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_FALSE(client.IsCached(tile_key));
    ASSERT_FALSE(client.IsCached(other_tile_key));
  }
  ASSERT_TRUE(cache->Clear());
}

}  // namespace
