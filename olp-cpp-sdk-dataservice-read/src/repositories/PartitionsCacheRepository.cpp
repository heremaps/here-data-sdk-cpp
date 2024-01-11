/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include "PartitionsCacheRepository.h"

#include <algorithm>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/cache/KeyGenerator.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/logging/Log.h>
// clang-format off
#include "generated/parser/PartitionsParser.h"
#include "generated/parser/LayerVersionsParser.h"
#include "JsonResultParser.h"
#include <olp/core/generated/parser/JsonParser.h>
#include <olp/core/generated/serializer/SerializerWrapper.h>
#include "generated/serializer/PartitionsSerializer.h"
#include "generated/serializer/LayerVersionsSerializer.h"
#include "generated/serializer/JsonSerializer.h"
// clang-format on

namespace {
constexpr auto kLogTag = "PartitionsCacheRepository";
constexpr auto kChronoSecondsMax = std::chrono::seconds::max();
constexpr auto kTimetMax = std::numeric_limits<time_t>::max();
constexpr auto kMaxQuadTreeIndexDepth = 4u;

time_t ConvertTime(std::chrono::seconds time) {
  return time == kChronoSecondsMax ? kTimetMax : time.count();
}
}  // namespace

namespace olp {
namespace dataservice {
namespace read {
namespace repository {
PartitionsCacheRepository::PartitionsCacheRepository(
    const client::HRN& catalog, const std::string& layer_id,
    std::shared_ptr<cache::KeyValueCache> cache,
    std::chrono::seconds default_expiry)
    : catalog_(catalog.ToCatalogHRNString()),
      layer_id_(layer_id),
      cache_(std::move(cache)),
      default_expiry_(ConvertTime(default_expiry)) {}

client::ApiNoResponse PartitionsCacheRepository::Put(
    const model::Partitions& partitions,
    const boost::optional<int64_t>& version,
    const boost::optional<time_t>& expiry, bool layer_metadata) {
  const auto& partitions_list = partitions.GetPartitions();
  std::vector<std::string> partition_ids;
  partition_ids.reserve(partitions_list.size());

  for (const auto& partition : partitions_list) {
    const auto key = cache::KeyGenerator::CreatePartitionKey(
        catalog_, layer_id_, partition.GetPartition(), version);
    OLP_SDK_LOG_DEBUG_F(kLogTag, "Put -> '%s'", key.c_str());

    const auto put_result = cache_->Put(
        key, partition, [&]() { return serializer::serialize(partition); },
        expiry.get_value_or(default_expiry_));

    if (!put_result) {
      OLP_SDK_LOG_ERROR_F(kLogTag, "Failed to write -> '%s'", key.c_str());
      return {{client::ErrorCode::CacheIO, "Put to cache failed"}};
    }

    if (layer_metadata) {
      partition_ids.push_back(partition.GetPartition());
    }
  }

  if (layer_metadata) {
    const auto key =
        cache::KeyGenerator::CreatePartitionsKey(catalog_, layer_id_, version);
    OLP_SDK_LOG_DEBUG_F(kLogTag, "Put -> '%s'", key.c_str());

    const auto put_result =
        cache_->Put(key, partition_ids,
                    [&]() { return serializer::serialize(partition_ids); },
                    expiry.get_value_or(default_expiry_));

    if (!put_result) {
      OLP_SDK_LOG_ERROR_F(kLogTag, "Failed to write -> '%s'", key.c_str());
      return {{client::ErrorCode::CacheIO, "Put to cache failed"}};
    }
  }

  return {client::ApiNoResult{}};
}

model::Partitions PartitionsCacheRepository::Get(
    const std::vector<std::string>& partition_ids,
    const boost::optional<int64_t>& version) {
  model::Partitions cached_partitions_model;
  auto& cached_partitions = cached_partitions_model.GetMutablePartitions();
  cached_partitions.reserve(partition_ids.size());

  for (const auto& partition_id : partition_ids) {
    const auto key = cache::KeyGenerator::CreatePartitionKey(
        catalog_, layer_id_, partition_id, version);
    OLP_SDK_LOG_DEBUG_F(kLogTag, "Get '%s'", key.c_str());

    auto cached_partition =
        cache_->Get(key, [](const std::string& serialized_object) {
          return parser::parse<model::Partition>(serialized_object);
        });

    if (!cached_partition.empty()) {
      cached_partitions.emplace_back(
          std::move(boost::any_cast<model::Partition&&>(cached_partition)));
    }
  }

  cached_partitions.shrink_to_fit();
  return cached_partitions_model;
}

boost::optional<model::Partitions> PartitionsCacheRepository::Get(
    const PartitionsRequest& request, const boost::optional<int64_t>& version) {
  const auto key =
      cache::KeyGenerator::CreatePartitionsKey(catalog_, layer_id_, version);
  boost::optional<model::Partitions> partitions;
  const auto& partition_ids = request.GetPartitionIds();

  if (partition_ids.empty()) {
    auto cached_ids = cache_->Get(key, [](const std::string& serialized_ids) {
      return parser::parse<std::vector<std::string>>(serialized_ids);
    });

    partitions =
        cached_ids.empty()
            ? boost::none
            : boost::optional<model::Partitions>(
                  Get(boost::any_cast<std::vector<std::string>>(cached_ids),
                      version));

  } else {
    auto available_partitions = Get(partition_ids, version);
    // In the case when not all partitions are available, we fail the cache
    // lookup. This can be enhanced in the future.
    if (available_partitions.GetPartitions().size() != partition_ids.size()) {
      partitions = boost::none;
    } else {
      partitions = std::move(available_partitions);
    }
  }

  return partitions;
}

void PartitionsCacheRepository::Put(
    int64_t catalog_version, const model::LayerVersions& layer_versions) {
  const auto key =
      cache::KeyGenerator::CreateLayerVersionsKey(catalog_, catalog_version);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "Put -> '%s'", key.c_str());

  cache_->Put(key, layer_versions,
              [&]() { return serializer::serialize(layer_versions); },
              default_expiry_);
}

boost::optional<model::LayerVersions> PartitionsCacheRepository::Get(
    int64_t catalog_version) {
  const auto key =
      cache::KeyGenerator::CreateLayerVersionsKey(catalog_, catalog_version);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "Get -> '%s'", key.c_str());

  auto cached_layer_versions =
      cache_->Get(key, [](const std::string& serialized_object) {
        return parser::parse<model::LayerVersions>(serialized_object);
      });

  if (cached_layer_versions.empty()) {
    return boost::none;
  }

  return std::move(
      boost::any_cast<model::LayerVersions&&>(cached_layer_versions));
}

client::ApiNoResponse PartitionsCacheRepository::Put(
    geo::TileKey tile_key, int32_t depth, const QuadTreeIndex& quad_tree,
    const boost::optional<int64_t>& version) {
  const auto key = cache::KeyGenerator::CreateQuadTreeKey(
      catalog_, layer_id_, tile_key, version, depth);

  if (quad_tree.IsNull()) {
    OLP_SDK_LOG_WARNING_F(kLogTag, "Put: invalid QuadTreeIndex -> '%s'",
                          key.c_str());
    return {client::ApiNoResult{}};
  }

  OLP_SDK_LOG_DEBUG_F(kLogTag, "Put -> '%s'", key.c_str());

  if (!cache_->Put(key, quad_tree.GetRawData(), default_expiry_)) {
    OLP_SDK_LOG_WARNING_F(kLogTag, "Failed to write -> '%s'", key.c_str());
    return {{client::ErrorCode::CacheIO, "Put to cache failed"}};
  }

  return {client::ApiNoResult{}};
}

bool PartitionsCacheRepository::Get(geo::TileKey tile_key, int32_t depth,
                                    const boost::optional<int64_t>& version,
                                    QuadTreeIndex& tree) {
  const auto key = cache::KeyGenerator::CreateQuadTreeKey(
      catalog_, layer_id_, tile_key, version, depth);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "Get -> '%s'", key.c_str());

  auto data = cache_->Get(key);
  if (data) {
    tree = QuadTreeIndex(data);
    return true;
  }

  return false;
}

void PartitionsCacheRepository::Clear() {
  auto key = catalog_ + "::" + layer_id_ + "::";
  OLP_SDK_LOG_INFO_F(kLogTag, "Clear -> '%s'", key.c_str());
  cache_->RemoveKeysWithPrefix(key);
}

void PartitionsCacheRepository::ClearPartitions(
    const std::vector<std::string>& partition_ids,
    const boost::optional<int64_t>& version) {
  OLP_SDK_LOG_INFO_F(kLogTag, "ClearPartitions -> '%s'", catalog_.c_str());
  auto cached_partitions = Get(partition_ids, version);

  // Partitions not processed here are not cached to begin with.
  for (const auto& partition : cached_partitions.GetPartitions()) {
    cache_->RemoveKeysWithPrefix(catalog_ + "::" + layer_id_ +
                                 "::" + partition.GetDataHandle());
    cache_->RemoveKeysWithPrefix(catalog_ + "::" + layer_id_ +
                                 "::" + partition.GetPartition());
  }
}

bool PartitionsCacheRepository::ClearQuadTree(
    geo::TileKey tile_key, int32_t depth,
    const boost::optional<int64_t>& version) {
  const auto key = cache::KeyGenerator::CreateQuadTreeKey(
      catalog_, layer_id_, tile_key, version, depth);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "ClearQuadTree -> '%s'", key.c_str());

  return cache_->RemoveKeysWithPrefix(key);
}

bool PartitionsCacheRepository::ClearPartitionMetadata(
    const std::string& partition_id,
    const boost::optional<int64_t>& catalog_version,
    boost::optional<model::Partition>& out_partition) {
  const auto key = cache::KeyGenerator::CreatePartitionKey(
      catalog_, layer_id_, partition_id, catalog_version);
  OLP_SDK_LOG_INFO_F(kLogTag, "ClearPartitionMetadata -> '%s'", key.c_str());

  auto cached_partition =
      cache_->Get(key, [](const std::string& serialized_object) {
        return parser::parse<model::Partition>(serialized_object);
      });

  if (cached_partition.empty()) {
    return true;
  }

  out_partition =
      std::move(boost::any_cast<model::Partition&&>(cached_partition));
  return cache_->RemoveKeysWithPrefix(key);
}

bool PartitionsCacheRepository::GetPartitionHandle(
    const std::string& partition_id,
    const boost::optional<int64_t>& catalog_version, std::string& data_handle) {
  const auto key = cache::KeyGenerator::CreatePartitionKey(
      catalog_, layer_id_, partition_id, catalog_version);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "IsPartitionCached -> '%s'", key.c_str());

  // Memory cache may save whole boost::any so sometimes there are no parsing
  auto cached_partition =
      cache_->Get(key, [](const std::string& serialized_object) {
        return parser::parse<model::Partition>(serialized_object);
      });

  if (cached_partition.empty()) {
    return false;
  }
  auto& partition = boost::any_cast<model::Partition&>(cached_partition);
  data_handle = std::move(partition.GetMutableDataHandle());
  return true;
}

bool PartitionsCacheRepository::FindQuadTree(geo::TileKey key,
                                             boost::optional<int64_t> version,
                                             read::QuadTreeIndex& tree) {
  auto max_depth = std::min<std::uint32_t>(key.Level(), kMaxQuadTreeIndexDepth);
  for (int32_t i = max_depth; i >= 0; i--) {
    const auto& root_tile_key = key.ChangedLevelBy(-i);
    QuadTreeIndex cached_tree;
    if (Get(root_tile_key, kMaxQuadTreeIndexDepth, version, cached_tree)) {
      OLP_SDK_LOG_DEBUG_F(kLogTag,
                          "FindQuadTree found in cache, tile='%s', "
                          "root='%s', depth='%" PRId32 "'",
                          key.ToHereTile().c_str(),
                          root_tile_key.ToHereTile().c_str(),
                          kMaxQuadTreeIndexDepth);
      tree = std::move(cached_tree);

      return true;
    }
  }

  return false;
}

bool PartitionsCacheRepository::ContainsTree(
    geo::TileKey key, int32_t depth,
    const boost::optional<int64_t>& version) const {
  return cache_->Contains(cache::KeyGenerator::CreateQuadTreeKey(
      catalog_, layer_id_, key, version, depth));
}

cache::KeyValueCache::KeyListType
PartitionsCacheRepository::CreatePartitionKeys(
    const std::string& partition_id, const boost::optional<int64_t>& version) {
  std::string handle;

  if (GetPartitionHandle(partition_id, version, handle)) {
    return cache::KeyValueCache::KeyListType{
        cache::KeyGenerator::CreatePartitionKey(catalog_, layer_id_,
                                                partition_id, version),
        cache::KeyGenerator::CreateDataHandleKey(catalog_, layer_id_, handle)};
  }

  return {};
}

bool PartitionsCacheRepository::Protect(
    const std::vector<std::string>& partition_ids,
    const boost::optional<int64_t>& version) {
  cache::KeyValueCache::KeyListType keys;
  keys.reserve(partition_ids.size() * 2u);
  std::for_each(partition_ids.cbegin(), partition_ids.cend(),
                [&](const std::string& partition_id) {
                  auto partition_keys =
                      CreatePartitionKeys(partition_id, version);
                  std::move(partition_keys.begin(), partition_keys.end(),
                            std::back_inserter(keys));
                });

  if (keys.empty()) {
    return false;
  }

  return cache_->Protect(keys);
}

bool PartitionsCacheRepository::Release(
    const std::vector<std::string>& partition_ids,
    const boost::optional<int64_t>& version) {
  cache::KeyValueCache::KeyListType keys;
  keys.reserve(partition_ids.size() * 2u);
  std::for_each(partition_ids.cbegin(), partition_ids.cend(),
                [&](const std::string& partition_id) {
                  auto partition_keys =
                      CreatePartitionKeys(partition_id, version);
                  std::move(partition_keys.begin(), partition_keys.end(),
                            std::back_inserter(keys));
                });

  if (keys.empty()) {
    return false;
  }

  return cache_->Release(keys);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
