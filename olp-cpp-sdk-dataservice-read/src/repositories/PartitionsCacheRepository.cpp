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

#include <string>

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

namespace {
constexpr auto kLogTag = "PartitionsCacheRepository";

std::string CreateKey(const std::string& hrn, const std::string& layerId,
                      const std::string& partitionId,
                      const boost::optional<int64_t>& version) {
  return hrn + "::" + layerId + "::" + partitionId +
         "::" + (version ? std::to_string(*version) + "::partition" : "");
}
std::string CreateKey(const std::string& hrn, const std::string& layerId,
                      const boost::optional<int64_t>& version) {
  return hrn + "::" + layerId +
         "::" + (version ? std::to_string(*version) + "::partitions" : "");
}
std::string CreateKey(const std::string& hrn, const int64_t catalogVersion) {
  return hrn + "::" + std::to_string(catalogVersion) + "::layerVersions";
}
}  // namespace

namespace olp {
namespace dataservice {
namespace read {
namespace repository {
using namespace olp::client;
PartitionsCacheRepository::PartitionsCacheRepository(
    const HRN& hrn, std::shared_ptr<cache::KeyValueCache> cache)
    : hrn_(hrn), cache_(cache) {}

void PartitionsCacheRepository::Put(const PartitionsRequest& request,
                                    const model::Partitions& partitions,
                                    const boost::optional<time_t>& expiry,
                                    bool allLayer) {
  std::string hrn(hrn_.ToCatalogHRNString());
  EDGE_SDK_LOG_TRACE_F(kLogTag, "Put '%s'", hrn.c_str());
  std::vector<std::string> partitionIds;
  time_t no_expiry = std::numeric_limits<time_t>::max();

  for (auto partition : partitions.GetPartitions()) {
    auto key = CreateKey(hrn, request.GetLayerId(), partition.GetPartition(),
                         request.GetVersion());
    EDGE_SDK_LOG_INFO_F(kLogTag, "Put '%s'", key.c_str());
    cache_->Put(key, partition,
                [=]() { return olp::serializer::serialize(partition); },
                expiry.get_value_or(no_expiry));
    if (allLayer) {
      partitionIds.push_back(partition.GetPartition());
    }
  }
  if (allLayer) {
    auto key = CreateKey(hrn, request.GetLayerId(), request.GetVersion());
    EDGE_SDK_LOG_INFO_F(kLogTag, "Put '%s'", key.c_str());
    cache_->Put(key, partitionIds,
                [=]() { return olp::serializer::serialize(partitionIds); },
                expiry.get_value_or(no_expiry));
  }
}

model::Partitions PartitionsCacheRepository::Get(
    const PartitionsRequest& request,
    const std::vector<std::string>& partitionIds) {
  std::string hrn(hrn_.ToCatalogHRNString());
  EDGE_SDK_LOG_TRACE_F(kLogTag, "Get '%s'", hrn.c_str());
  model::Partitions cachedPartitionsModel;
  std::vector<model::Partition> cachedPartitions;
  for (auto partitionId : partitionIds) {
    auto key =
        CreateKey(hrn, request.GetLayerId(), partitionId, request.GetVersion());
    EDGE_SDK_LOG_INFO_F(kLogTag, "Get '%s'", key.c_str());
    auto cachedPartition =
        cache_->Get(key, [](const std::string& serializedObject) {
          return parser::parse<model::Partition>(serializedObject);
        });
    if (!cachedPartition.empty()) {
      cachedPartitions.push_back(
          boost::any_cast<model::Partition>(cachedPartition));
    }
  }
  cachedPartitionsModel.SetPartitions(cachedPartitions);
  return cachedPartitionsModel;
}

boost::optional<model::Partitions> PartitionsCacheRepository::Get(
    const PartitionsRequest& request) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn, request.GetLayerId(), request.GetVersion());
  EDGE_SDK_LOG_TRACE_F(kLogTag, "Get '%s'", key.c_str());
  auto cachedIds = cache_->Get(key, [](const std::string& serializedIds) {
    return parser::parse<std::vector<std::string>>(serializedIds);
  });
  if (cachedIds.empty()) {
    return boost::none;
  }

  return Get(request, boost::any_cast<std::vector<std::string>>(cachedIds));
}

void PartitionsCacheRepository::Put(int64_t catalogVersion,
                                    const model::LayerVersions& layerVersions) {
  std::string hrn(hrn_.ToCatalogHRNString());
  EDGE_SDK_LOG_INFO_F(kLogTag, "Put '%s'", hrn.c_str());
  cache_->Put(CreateKey(hrn, catalogVersion), layerVersions,
              [=]() { return olp::serializer::serialize(layerVersions); });
}

boost::optional<model::LayerVersions> PartitionsCacheRepository::Get(
    int64_t catalogVersion) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn, catalogVersion);
  EDGE_SDK_LOG_INFO_F(kLogTag, "Get '%s'", key.c_str());
  auto cachedLayerVersions =
      cache_->Get(key, [](const std::string& serializedObject) {
        return parser::parse<model::LayerVersions>(serializedObject);
      });
  if (cachedLayerVersions.empty()) {
    return boost::none;
  }
  return boost::any_cast<model::LayerVersions>(cachedLayerVersions);
}

void PartitionsCacheRepository::Clear(const std::string& layer_id) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn, layer_id, boost::none);
  EDGE_SDK_LOG_INFO_F(kLogTag, "Clear '%s'", key.c_str());
  cache_->RemoveKeysWithPrefix(key);
}

void PartitionsCacheRepository::ClearPartitions(
    const PartitionsRequest& request,
    const std::vector<std::string>& partitionIds) {
  std::string hrn(hrn_.ToCatalogHRNString());
  EDGE_SDK_LOG_INFO_F(kLogTag, "ClearPartitions '%s'", hrn.c_str());
  auto cachedPartitions = Get(request, partitionIds);
  // Partitions not processed here are not cached to begin with.
  for (auto partition : cachedPartitions.GetPartitions()) {
    cache_->RemoveKeysWithPrefix(hrn + "::" + request.GetLayerId() +
                                 "::" + partition.GetDataHandle());
    cache_->RemoveKeysWithPrefix(hrn + "::" + request.GetLayerId() +
                                 "::" + partition.GetPartition());
  }
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
