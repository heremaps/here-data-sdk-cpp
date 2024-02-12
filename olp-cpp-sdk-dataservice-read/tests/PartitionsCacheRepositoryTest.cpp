/*
 * Copyright (C) 2020-2024 HERE Europe B.V.
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

#include "repositories/PartitionsCacheRepository.h"

#include <gmock/gmock.h>
#include <mocks/CacheMock.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include "repositories/DataCacheRepository.h"

namespace {
namespace read = olp::dataservice::read;
namespace model = olp::dataservice::read::model;
namespace repository = olp::dataservice::read::repository;
using olp::cache::KeyValueCache;
using olp::client::HRN;

constexpr auto kCatalog = "hrn:here:data::olp-here-test:catalog";
constexpr auto kPartitionId = "1111";
constexpr auto kDataHandle = "7636348E50215979A39B5F3A429EDDB4.1111";
constexpr auto kHereTile = "23618364";
constexpr auto kQuadkeyResponse =
    R"jsonString({"subQuads": [{"subQuadKey": "4","version":282,"dataHandle":"7636348E50215979A39B5F3A429EDDB4.282","dataSize":277},{"subQuadKey":"5","version":282,"dataHandle":"8C9B3E08E294ADB2CD07EBC8412062FE.282","dataSize":271},{"subQuadKey": "6","version":282,"dataHandle":"9772F5E1822DFF25F48F150294B1ECF5.282","dataSize":289},{"subQuadKey":"7","version":282,"dataHandle":"BF84D8EC8124B96DBE5C4DB68B05918F.282","dataSize":283},{"subQuadKey":"1","version":48,"dataHandle":"BD53A6D60A34C20DC42ACAB2650FE361.48","dataSize":89}],"parentQuads":[{"partition":"23","version":282,"dataHandle":"F8F4C3CB09FBA61B927256CBCB8441D1.282","dataSize":52438},{"partition":"5","version":282,"dataHandle":"13E2C624E0136C3357D092EE7F231E87.282","dataSize":99151},{"partition":"95","version":253,"dataHandle":"B6F7614316BB8B81478ED7AE370B22A6.253","dataSize":6765}]})jsonString";

TEST(PartitionsCacheRepositoryTest, DefaultExpiry) {
  const auto hrn = HRN::FromString(kCatalog);
  const auto layer = "layer";
  const auto catalog_version = 0;

  const auto request = read::PartitionsRequest();
  model::Partition some_partition;
  some_partition.SetPartition(kPartitionId);
  model::Partitions partitions;
  auto& partitions_vector = partitions.GetMutablePartitions();
  partitions_vector.push_back(some_partition);

  model::LayerVersion layer_version;
  layer_version.SetLayer(layer);
  model::LayerVersions versions;
  auto& versions_vector = versions.GetMutableLayerVersions();
  versions_vector.push_back(layer_version);

  {
    SCOPED_TRACE("Disable expiration");

    const auto default_expiry = std::chrono::seconds::max();
    std::shared_ptr<KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::PartitionsCacheRepository repository(hrn, layer, cache,
                                                     default_expiry);

    repository.Put(partitions, boost::none, boost::none, true);
    repository.Put(catalog_version, versions);
    const auto partitions_result = repository.Get({kPartitionId}, boost::none);
    const auto partitions_optional_result =
        repository.Get(request, boost::none);
    const auto versions_result = repository.Get(catalog_version);

    EXPECT_FALSE(partitions_result.GetPartitions().empty());
    EXPECT_TRUE(partitions_optional_result);
    EXPECT_TRUE(versions_result);
  }

  {
    SCOPED_TRACE("Expired");

    const auto default_expiry = std::chrono::seconds(-1);
    std::shared_ptr<KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::PartitionsCacheRepository repository(hrn, layer, cache,
                                                     default_expiry);

    repository.Put(partitions, boost::none, boost::none, true);
    repository.Put(catalog_version, versions);

    const auto partitions_result = repository.Get({kPartitionId}, boost::none);
    const auto partitions_optional_result =
        repository.Get(request, boost::none);
    const auto versions_result = repository.Get(catalog_version);

    EXPECT_TRUE(partitions_result.GetPartitions().empty());
    EXPECT_FALSE(partitions_optional_result);
    EXPECT_FALSE(versions_result);
  }

  {
    SCOPED_TRACE("Optional not expired");

    const auto default_expiry = std::chrono::seconds(-1);
    const auto data_expiry = std::numeric_limits<time_t>::max();
    std::shared_ptr<KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::PartitionsCacheRepository repository(hrn, layer, cache,
                                                     default_expiry);

    repository.Put(partitions, boost::none, data_expiry, true);
    const auto partitions_result = repository.Get({kPartitionId}, boost::none);
    const auto optional_result = repository.Get(request, boost::none);

    EXPECT_FALSE(partitions_result.GetPartitions().empty());
    EXPECT_TRUE(optional_result);
  }

  {
    SCOPED_TRACE("Optional expired");

    const auto default_expiry = std::chrono::seconds::max();
    const auto data_expiry = time_t(-1);
    std::shared_ptr<KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::PartitionsCacheRepository repository(hrn, layer, cache,
                                                     default_expiry);

    repository.Put(partitions, boost::none, data_expiry, true);
    const auto partitions_result = repository.Get({kPartitionId}, boost::none);
    const auto optional_result = repository.Get(request, boost::none);

    EXPECT_TRUE(partitions_result.GetPartitions().empty());
    EXPECT_FALSE(optional_result);
  }
}

TEST(PartitionsCacheRepositoryTest, QuadTree) {
  using testing::_;
  using testing::DoAll;
  using testing::Return;
  using testing::SaveArg;

  const auto hrn = HRN::FromString(kCatalog);
  const auto layer = "layer";
  const auto version = 0;
  const auto tile_key = olp::geo::TileKey::FromHereTile(kHereTile);
  const auto depth = 2;

  {
    SCOPED_TRACE("Put/Get quad tree");

    auto stream = std::stringstream(kQuadkeyResponse);
    read::QuadTreeIndex quad_tree(tile_key, depth, stream);
    auto cache = std::make_shared<CacheMock>();
    repository::PartitionsCacheRepository repository(hrn, layer, cache);
    std::string key;

    EXPECT_CALL(*cache, Put(_, _, _))
        .WillOnce(DoAll(SaveArg<0>(&key), Return(true)));
    repository.Put(tile_key, depth, quad_tree, version);

    EXPECT_CALL(*cache, Get(key)).WillOnce(Return(quad_tree.GetRawData()));
    read::QuadTreeIndex tree;
    const auto result = repository.Get(tile_key, depth, version, tree);

    ASSERT_TRUE(result);
    ASSERT_FALSE(tree.IsNull());
    ASSERT_EQ(*tree.GetRawData(), *quad_tree.GetRawData());
  }

  {
    SCOPED_TRACE("Empty quad tree");

    read::QuadTreeIndex quad_tree;
    auto cache = std::make_shared<CacheMock>();
    repository::PartitionsCacheRepository repository(hrn, layer, cache);

    EXPECT_CALL(*cache, Get(_)).WillOnce(Return(KeyValueCache::ValueTypePtr()));

    repository.Put(tile_key, depth, quad_tree, version);
    read::QuadTreeIndex tree;
    const auto result = repository.Get(tile_key, depth, version, tree);

    ASSERT_FALSE(result);
    ASSERT_TRUE(tree.IsNull());
  }
}

TEST(PartitionsCacheRepositoryTest, GetPartitionHandle) {
  const auto hrn = HRN::FromString(kCatalog);
  const auto layer = "layer";

  model::Partition some_partition;
  some_partition.SetPartition(kPartitionId);
  some_partition.SetDataHandle(kDataHandle);
  model::Partitions partitions;
  auto& partitions_vector = partitions.GetMutablePartitions();
  partitions_vector.push_back(some_partition);

  {
    SCOPED_TRACE("Put/Check partition");

    std::shared_ptr<KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::PartitionsCacheRepository repository(hrn, layer, cache);
    std::string handle;
    repository.Put(partitions, boost::none, true);
    EXPECT_TRUE(
        repository.GetPartitionHandle(kPartitionId, boost::none, handle));
  }

  {
    SCOPED_TRACE("Check not existing partition");

    std::shared_ptr<KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::PartitionsCacheRepository repository(hrn, layer, cache);
    std::string handle;
    EXPECT_FALSE(
        repository.GetPartitionHandle(kPartitionId, boost::none, handle));
  }
}

TEST(PartitionsCacheRepositoryTest, ClearPartitions) {
  const auto hrn = HRN::FromString(kCatalog);
  const auto layer = "layer";

  model::Partition some_partition;
  some_partition.SetPartition(kPartitionId);
  some_partition.SetDataHandle(kDataHandle);
  model::Partitions partitions;
  auto& partitions_vector = partitions.GetMutablePartitions();
  partitions_vector.push_back(some_partition);

  const std::string kPartitionDataHandle =
      std::string(kCatalog) + "::" + layer + "::" + kDataHandle;
  const std::string kPartition =
      std::string(kCatalog) + "::" + layer + "::" + kPartitionId;
  const std::string kPartitionKey = kPartition + "::partition";

  auto cache = std::make_shared<testing::NaggyMock<CacheMock>>();

  EXPECT_CALL(*cache, Put(testing::Eq(kPartitionKey), testing::_, testing::_,
                          testing::_))
      .WillRepeatedly(testing::Return(true));

  EXPECT_CALL(*cache, Get(testing::Eq(kPartitionKey), testing::_))
      .WillRepeatedly(testing::Return(some_partition));

  {
    SCOPED_TRACE("RemoveKeysWithPrefix data handle failed");

    EXPECT_CALL(*cache, RemoveKeysWithPrefix(testing::Eq(kPartitionDataHandle)))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*cache, RemoveKeysWithPrefix(testing::Eq(kPartition)))
        .WillOnce(testing::Return(true));

    repository::PartitionsCacheRepository repository(hrn, layer, cache);
    repository.Put(partitions, boost::none, true);
    EXPECT_FALSE(repository.ClearPartitions({kPartitionId}, boost::none));
  }

  {
    SCOPED_TRACE("RemoveKeysWithPrefix partition failed");

    EXPECT_CALL(*cache, RemoveKeysWithPrefix(testing::Eq(kPartitionDataHandle)))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*cache, RemoveKeysWithPrefix(testing::Eq(kPartition)))
        .WillOnce(testing::Return(false));

    repository::PartitionsCacheRepository repository(hrn, layer, cache);
    repository.Put(partitions, boost::none, true);
    EXPECT_FALSE(repository.ClearPartitions({kPartitionId}, boost::none));
  }

  {
    SCOPED_TRACE("RemoveKeysWithPrefix passed");

    EXPECT_CALL(*cache, RemoveKeysWithPrefix(testing::Eq(kPartitionDataHandle)))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*cache, RemoveKeysWithPrefix(testing::Eq(kPartition)))
        .WillOnce(testing::Return(true));

    repository::PartitionsCacheRepository repository(hrn, layer, cache);
    repository.Put(partitions, boost::none, true);
    EXPECT_TRUE(repository.ClearPartitions({kPartitionId}, boost::none));
  }
}

}  // namespace
