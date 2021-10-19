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

#pragma once

#include <string>

#include <boost/optional.hpp>

#include <olp/core/CoreApi.h>
#include <olp/core/client/HRN.h>
#include <olp/core/geo/tiling/TileKey.h>

namespace olp {
namespace cache {

/**
 * @brief Helper class to generate cache keys for different entities.
 */
class CORE_API CacheKeyGenerator {
 public:
  /**
   * @brief Generates cache key for service API.
   *
   * @param hrn The HRN of the catalog.
   * @param service The service.
   * @param version The version of the service.
   *
   * @return A key used to store the API in cache.
   */
  static std::string CreateApiKey(const std::string& hrn,
                                  const std::string& service,
                                  const std::string& version);

  /**
   * @brief Generates cache key for catalog data.
   *
   * @param hrn The HRN of the catalog.
   *
   * @return A key used to store the catalog data in cache.
   */
  static std::string CreateCatalogKey(const std::string& hrn);

  /**
   * @brief Generates cache key to store latest catalog version.
   *
   * @param hrn The HRN of the catalog.
   *
   * @return A key used to store the version in cache.
   */
  static std::string CreateLatestVersionKey(const std::string& hrn);

  /**
   * @brief Generates cache key for storing partition data.
   *
   * @param hrn The HRN of the catalog.
   * @param layer_id The layer of the partition.
   * @param partition_id The partition name.
   * @param version The version of the catalog.
   *
   * @return A key used to store the patition data in cache.
   */
  static std::string CreatePartitionKey(
      const std::string& hrn, const std::string& layer_id,
      const std::string& partition_id, const boost::optional<int64_t>& version);

  /**
   * @brief Generates cache key for storing list of partitions.
   *
   * @param hrn The HRN of the catalog.
   * @param layer_id The layer of the partition.
   * @param version The version of the catalog.
   *
   * @return A key used to store the list of patitions in cache.
   */
  static std::string CreatePartitionsKey(
      const std::string& hrn, const std::string& layer_id,
      const boost::optional<int64_t>& version);

  /**
   * @brief Generates cache key for storing list of available layer versions.
   *
   * @param hrn The HRN of the catalog.
   * @param version The version of the catalog.
   *
   * @return A key used to store the list layer versions in cache.
   */
  static std::string CreateLayerVersionsKey(const std::string& hrn,
                                            const int64_t version);

  /**
   * @brief Generates cache key for storing quadtree metadata.
   *
   * @param hrn The HRN of the catalog.
   * @param layer_id The layer of the quadtree.
   * @param root The root tile of the quadtree.
   * @param version The version of the catalog.
   * @param depth The quadtree depth.
   *
   * @return A key used to store the quadtree in cache.
   */
  static std::string CreateQuadTreeKey(const std::string& hrn,
                                       const std::string& layer_id,
                                       olp::geo::TileKey root,
                                       const boost::optional<int64_t>& version,
                                       int32_t depth);

  /**
   * @brief Generates cache key for data handle entities.
   *
   * @param hrn The HRN of the catalog.
   * @param layer_id The layer of the data handle.
   * @param data_handle The data handle.
   *
   * @return A key used to store the data handle in cache.
   */
  static std::string CreateDataHandleKey(const std::string& hrn,
                                         const std::string& layer_id,
                                         const std::string& data_handle);
};

}  // namespace cache
}  // namespace olp
