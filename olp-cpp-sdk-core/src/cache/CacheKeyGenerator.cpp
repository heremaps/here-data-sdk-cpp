/*
 * Copyright (C) 2021 HERE Europe B.V.
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

#include "olp/core/cache/CacheKeyGenerator.h"

namespace olp {
namespace cache {
std::string CacheKeyGenerator::CreateApiKey(const std::string& hrn,
                                            const std::string& service,
                                            const std::string& version) {
  return hrn + "::" + service + "::" + version + "::api";
}

std::string CacheKeyGenerator::CreateCatalogKey(const std::string& hrn) {
  return hrn + "::catalog";
}

std::string CacheKeyGenerator::CreateLatestVersionKey(const std::string& hrn) {
  return hrn + "::latestVersion";
}

std::string CacheKeyGenerator::CreatePartitionKey(
    const std::string& hrn, const std::string& layer_id,
    const std::string& partition_id, const boost::optional<int64_t>& version) {
  return hrn + "::" + layer_id + "::" + partition_id +
         "::" + (version ? std::to_string(*version) + "::" : "") + "partition";
}

std::string CacheKeyGenerator::CreatePartitionsKey(
    const std::string& hrn, const std::string& layer_id,
    const boost::optional<int64_t>& version) {
  return hrn + "::" + layer_id +
         "::" + (version ? std::to_string(*version) + "::" : "") + "partitions";
}

std::string CacheKeyGenerator::CreateLayerVersionsKey(const std::string& hrn,
                                                      const int64_t version) {
  return hrn + "::" + std::to_string(version) + "::layerVersions";
}

std::string CacheKeyGenerator::CreateQuadTreeKey(
    const std::string& hrn, const std::string& layer_id, olp::geo::TileKey root,
    const boost::optional<int64_t>& version, int32_t depth) {
  return hrn + "::" + layer_id + "::" + root.ToHereTile() +
         "::" + (version ? std::to_string(*version) + "::" : "") +
         std::to_string(depth) + "::quadtree";
}

std::string CacheKeyGenerator::CreateDataHandleKey(
    const std::string& hrn, const std::string& layer_id,
    const std::string& data_handle) {
  return hrn + "::" + layer_id + "::" + data_handle + "::Data";
}

}  // namespace cache
}  // namespace olp
