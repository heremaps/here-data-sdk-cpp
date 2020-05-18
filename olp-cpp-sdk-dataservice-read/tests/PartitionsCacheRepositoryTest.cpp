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

#include "repositories/PartitionsCacheRepository.h"

#include <gmock/gmock.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>

namespace {

using namespace olp;
using namespace olp::dataservice::read;

constexpr auto kCatalog = "hrn:here:data::olp-here-test:catalog";
constexpr auto kPartitionId = "1111";

TEST(PartitionsCacheRepositoryTest, DefaultExpiry) {
  const auto hrn = client::HRN::FromString(kCatalog);
  const auto layer = "layer";
  const auto catalog_version = 0;

  const auto request = PartitionsRequest();
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
    std::shared_ptr<cache::KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::PartitionsCacheRepository repository(hrn, cache,
                                                     default_expiry);

    repository.Put(request, partitions, layer, boost::none, true);
    repository.Put(catalog_version, versions);
    const auto partitions_result =
        repository.Get(request, {kPartitionId}, layer);
    const auto partitions_optional_result = repository.Get(request, layer);
    const auto versions_result = repository.Get(catalog_version);

    EXPECT_FALSE(partitions_result.GetPartitions().empty());
    EXPECT_TRUE(partitions_optional_result);
    EXPECT_TRUE(versions_result);
  }

  {
    SCOPED_TRACE("Expired");

    const auto default_expiry = std::chrono::seconds(-1);
    std::shared_ptr<cache::KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::PartitionsCacheRepository repository(hrn, cache,
                                                     default_expiry);

    repository.Put(request, partitions, layer, boost::none, true);
    repository.Put(catalog_version, versions);

    const auto partitions_result =
        repository.Get(request, {kPartitionId}, layer);
    const auto partitions_optional_result = repository.Get(request, layer);
    const auto versions_result = repository.Get(catalog_version);

    EXPECT_TRUE(partitions_result.GetPartitions().empty());
    EXPECT_FALSE(partitions_optional_result);
    EXPECT_FALSE(versions_result);
  }

  {
    SCOPED_TRACE("Optional not expired");

    const auto default_expiry = std::chrono::seconds(-1);
    const auto data_expiry = std::numeric_limits<time_t>::max();
    std::shared_ptr<cache::KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::PartitionsCacheRepository repository(hrn, cache,
                                                     default_expiry);

    repository.Put(request, partitions, layer, data_expiry, true);
    const auto partitions_result =
        repository.Get(request, {kPartitionId}, layer);
    const auto optional_result = repository.Get(request, layer);

    EXPECT_FALSE(partitions_result.GetPartitions().empty());
    EXPECT_TRUE(optional_result);
  }

  {
    SCOPED_TRACE("Optional expired");

    const auto default_expiry = std::chrono::seconds::max();
    const auto data_expiry = time_t(-1);
    std::shared_ptr<cache::KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::PartitionsCacheRepository repository(hrn, cache,
                                                     default_expiry);

    repository.Put(request, partitions, layer, data_expiry, true);
    const auto partitions_result =
        repository.Get(request, {kPartitionId}, layer);
    const auto optional_result = repository.Get(request, layer);

    EXPECT_TRUE(partitions_result.GetPartitions().empty());
    EXPECT_FALSE(optional_result);
  }
}

}  // namespace
