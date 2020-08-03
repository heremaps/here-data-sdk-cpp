/*
 * Copyright (C) 2019 HERE Europe B.V.
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

#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/logging/Log.h>
// clang-format off
#include "generated/parser/PartitionsParser.h"
#include "generated/parser/LayerVersionsParser.h"
#include <olp/core/generated/parser/JsonParser.h>
#include <olp/core/generated/serializer/SerializerWrapper.h>
#include "generated/serializer/PartitionsSerializer.h"
#include "generated/serializer/LayerVersionsSerializer.h"
#include "generated/serializer/JsonSerializer.h"
// clang-format on

// Needed to avoid endless warnings from GetVersion/WithVersion
#include <olp/core/porting/warning_disable.h>
PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

namespace {
constexpr auto kLogTag = "PartitionsCacheRepository";
constexpr auto kChronoSecondsMax = std::chrono::seconds::max();
constexpr auto kTimetMax = std::numeric_limits<time_t>::max();
constexpr uint32_t kMaxQuadTreeIndexDepth = 4u;

std::string CreateKey(const std::string& hrn, const std::string& layer_id,
                      const std::string& partitionId,
                      const boost::optional<int64_t>& version) {
  return hrn + "::" + layer_id + "::" + partitionId +
         "::" + (version ? std::to_string(*version) + "::" : "") + "partition";
}
std::string CreateKey(const std::string& hrn, const std::string& layer_id,
                      const boost::optional<int64_t>& version) {
  return hrn + "::" + layer_id +
         "::" + (version ? std::to_string(*version) + "::" : "") + "partitions";
}
std::string CreateKey(const std::string& hrn, const int64_t catalogVersion) {
  return hrn + "::" + std::to_string(catalogVersion) + "::layerVersions";
}

time_t ConvertTime(std::chrono::seconds time) {
  return time == kChronoSecondsMax ? kTimetMax : time.count();
}
}  // namespace

namespace olp {
namespace dataservice {
namespace read {
namespace repository {
PartitionsCacheRepository::PartitionsCacheRepository(
    const client::HRN& hrn, std::shared_ptr<cache::KeyValueCache> cache,
    std::chrono::seconds default_expiry)
    : hrn_(hrn), cache_(cache), default_expiry_(ConvertTime(default_expiry)) {}

void PartitionsCacheRepository::Put(const model::Partitions& partitions,
                                    const std::string& layer_id,
                                    const boost::optional<int64_t>& version,
                                    const boost::optional<time_t>& expiry,
                                    bool layer_metadata) {
  std::string hrn(hrn_.ToCatalogHRNString());
  const auto& partitions_list = partitions.GetPartitions();
  std::vector<std::string> partition_ids;
  partition_ids.reserve(partitions_list.size());

  for (const auto& partition : partitions_list) {
    auto key = CreateKey(hrn, layer_id, partition.GetPartition(), version);
    OLP_SDK_LOG_DEBUG_F(kLogTag, "Put -> '%s'", key.c_str());

    cache_->Put(key, partition,
                [&]() { return olp::serializer::serialize(partition); },
                expiry.get_value_or(default_expiry_));

    if (layer_metadata) {
      partition_ids.push_back(partition.GetPartition());
    }
  }

  if (layer_metadata) {
    auto key = CreateKey(hrn, layer_id, version);
    OLP_SDK_LOG_DEBUG_F(kLogTag, "Put -> '%s'", key.c_str());

    cache_->Put(key, partition_ids,
                [&]() { return olp::serializer::serialize(partition_ids); },
                expiry.get_value_or(default_expiry_));
  }
}

model::Partitions PartitionsCacheRepository::Get(
    const std::vector<std::string>& partition_ids, const std::string& layer_id,
    const boost::optional<int64_t>& version) {
  std::string hrn(hrn_.ToCatalogHRNString());
  model::Partitions cached_partitions_model;
  auto& cached_partitions = cached_partitions_model.GetMutablePartitions();
  cached_partitions.reserve(partition_ids.size());

  for (const auto& partition_id : partition_ids) {
    auto key = CreateKey(hrn, layer_id, partition_id, version);
    OLP_SDK_LOG_DEBUG_F(kLogTag, "Get '%s'", key.c_str());

    auto cached_partition =
        cache_->Get(key, [](const std::string& serialized_object) {
          return parser::parse<model::Partition>(serialized_object);
        });

    if (!cached_partition.empty()) {
      cached_partitions.emplace_back(
          boost::any_cast<model::Partition>(cached_partition));
    }
  }

  cached_partitions.shrink_to_fit();
  return cached_partitions_model;
}

boost::optional<model::Partitions> PartitionsCacheRepository::Get(
    const PartitionsRequest& request, const std::string& layer_id,
    const boost::optional<int64_t>& version) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn, layer_id, version);
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
                      layer_id, version));

  } else {
    auto available_partitions = Get(partition_ids, layer_id, version);
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
  std::string hrn(hrn_.ToCatalogHRNString());
  const auto key = CreateKey(hrn, catalog_version);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "Put -> '%s'", key.c_str());

  cache_->Put(key, layer_versions,
              [&]() { return olp::serializer::serialize(layer_versions); },
              default_expiry_);
}

boost::optional<model::LayerVersions> PartitionsCacheRepository::Get(
    int64_t catalog_version) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn, catalog_version);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "Get -> '%s'", key.c_str());

  auto cached_layer_versions =
      cache_->Get(key, [](const std::string& serialized_object) {
        return parser::parse<model::LayerVersions>(serialized_object);
      });

  if (cached_layer_versions.empty()) {
    return boost::none;
  }

  return boost::any_cast<model::LayerVersions>(cached_layer_versions);
}

void PartitionsCacheRepository::Put(const std::string& layer,
                                    geo::TileKey tile_key, int32_t depth,
                                    const QuadTreeIndex& quad_tree,
                                    const boost::optional<int64_t>& version) {
  const auto key = CreateQuadKey(layer, tile_key, depth, version);

  if (quad_tree.IsNull()) {
    OLP_SDK_LOG_WARNING_F(kLogTag, "Put: invalid QuadTreeIndex -> '%s'",
                          key.c_str());
    return;
  }

  OLP_SDK_LOG_DEBUG_F(kLogTag, "Put -> '%s'", key.c_str());
  cache_->Put(key, quad_tree.GetRawData(), default_expiry_);
}

bool PartitionsCacheRepository::Get(const std::string& layer,
                                    geo::TileKey tile_key, int32_t depth,
                                    const boost::optional<int64_t>& version,
                                    QuadTreeIndex& tree) {
  auto key = CreateQuadKey(layer, tile_key, depth, version);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "Get -> '%s'", key.c_str());
  auto data = cache_->Get(key);
  if (data) {
    tree = QuadTreeIndex(data);
    return true;
  }

  return false;
}

void PartitionsCacheRepository::Clear(const std::string& layer_id) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = hrn + "::" + layer_id + "::";
  OLP_SDK_LOG_INFO_F(kLogTag, "Clear -> '%s'", key.c_str());
  cache_->RemoveKeysWithPrefix(key);
}

void PartitionsCacheRepository::ClearPartitions(
    const std::vector<std::string>& partition_ids, const std::string& layer_id,
    const boost::optional<int64_t>& version) {
  std::string hrn(hrn_.ToCatalogHRNString());
  OLP_SDK_LOG_INFO_F(kLogTag, "ClearPartitions -> '%s'", hrn.c_str());
  auto cached_partitions = Get(partition_ids, layer_id, version);

  // Partitions not processed here are not cached to begin with.
  for (auto partition : cached_partitions.GetPartitions()) {
    cache_->RemoveKeysWithPrefix(hrn + "::" + layer_id +
                                 "::" + partition.GetDataHandle());
    cache_->RemoveKeysWithPrefix(hrn + "::" + layer_id +
                                 "::" + partition.GetPartition());
  }
}

bool PartitionsCacheRepository::ClearQuadTree(
    const std::string& layer, geo::TileKey tile_key, int32_t depth,
    const boost::optional<int64_t>& version) {
  const auto key = CreateQuadKey(layer, tile_key, depth, version);
  OLP_SDK_LOG_INFO_F(kLogTag, "ClearQuadTree -> '%s'", key.c_str());
  return cache_->RemoveKeysWithPrefix(key);
}

bool PartitionsCacheRepository::ClearPartitionMetadata(
    const boost::optional<int64_t>& catalog_version,
    const std::string& partition_id, const std::string& layer_id,
    boost::optional<model::Partition>& out_partition) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn, layer_id, partition_id, catalog_version);
  OLP_SDK_LOG_INFO_F(kLogTag, "ClearPartitionMetadata -> '%s'", key.c_str());

  auto cached_partition =
      cache_->Get(key, [](const std::string& serialized_object) {
        return parser::parse<model::Partition>(serialized_object);
      });

  if (cached_partition.empty()) {
    return true;
  }

  out_partition = boost::any_cast<model::Partition>(cached_partition);
  return cache_->RemoveKeysWithPrefix(key);
}

bool PartitionsCacheRepository::GetPartitionHandle(
    const boost::optional<int64_t>& catalog_version,
    const std::string& partition_id, const std::string& layer_id,
    std::string& data_handle) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn, layer_id, partition_id, catalog_version);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "IsPartitionCached -> '%s'", key.c_str());
  auto cached_partition =
      cache_->Get(key, [](const std::string& serialized_object) {
        return parser::parse<model::Partition>(serialized_object);
      });

  if (cached_partition.empty()) {
    return false;
  }
  auto partition = boost::any_cast<model::Partition>(cached_partition);
  data_handle = partition.GetDataHandle();
  return true;
}

std::string PartitionsCacheRepository::CreateQuadKey(
    const std::string& layer, olp::geo::TileKey key, int32_t depth,
    const boost::optional<int64_t>& version) const {
  std::string hrn(hrn_.ToCatalogHRNString());
  return hrn + "::" + layer + "::" + key.ToHereTile() +
         "::" + (version ? std::to_string(*version) + "::" : "") +
         std::to_string(depth) + "::quadtree";
}

bool PartitionsCacheRepository::FindQuadTree(const std::string& layer,
                                             boost::optional<int64_t> version,
                                             const olp::geo::TileKey& tile_key,
                                             read::QuadTreeIndex& tree) {
  auto max_depth =
      std::min<std::int32_t>(tile_key.Level(), kMaxQuadTreeIndexDepth);
  for (auto i = max_depth; i >= 0; --i) {
    const auto& root_tile_key = tile_key.ChangedLevelBy(-i);
    QuadTreeIndex cached_tree;
    if (Get(layer, root_tile_key, kMaxQuadTreeIndexDepth, version,
            cached_tree)) {
      OLP_SDK_LOG_DEBUG_F(kLogTag,
                          "FindQuadTree found in cache, tile='%s', "
                          "root='%s', depth='%" PRId32 "'",
                          tile_key.ToHereTile().c_str(),
                          root_tile_key.ToHereTile().c_str(),
                          kMaxQuadTreeIndexDepth);
      tree = std::move(cached_tree);

      return true;
    }
  }

  return false;
}

PORTING_POP_WARNINGS()
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
