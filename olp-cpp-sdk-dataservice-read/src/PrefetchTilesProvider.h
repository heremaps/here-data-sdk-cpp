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

#pragma once

#include <atomic>
#include <map>
#include <string>

#include <olp/core/geo/tiling/TileKey.h>

#include "olp/dataservice/read/CatalogClient.h"
#include "repositories/PrefetchTilesRepository.h"

namespace olp {
namespace dataservice {
namespace read {

namespace repository {
class ApiRepository;
class CatalogRepository;
class DataRepository;
}  // namespace repository

class PrefetchTilesProvider final {
 public:
  PrefetchTilesProvider(
      const client::HRN& hrn, std::string layer_id,
      std::shared_ptr<repository::ApiRepository> api_repo,
      std::shared_ptr<repository::CatalogRepository> catalog_repo,
      std::shared_ptr<repository::DataRepository> data_repo,
      std::shared_ptr<repository::PrefetchTilesRepository> prefetch_tiles_repo,
      std::shared_ptr<client::OlpClientSettings> settings);

  /**
   * @brief pre-fetches a set of tiles asynchronously
   *
   * Recursively downloads all tilekeys up to maxLevel.
   *
   * Note - this does not guarantee that all tiles are available offline, as the
   * cache might overflow and data might be evicted at any point.
   *
   * @param callback the callback to involve when the operation finished
   * @param statusCallback the callback to involve to indicate status of the
   * operation
   * @param billingTag billingTag is an optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @return A token that can be used to cancel this prefetch request
   */
  client::CancellationToken PrefetchTiles(
      const PrefetchTilesRequest& request,
      const PrefetchTilesResponseCallback& callback);

  static void QueryDataForEachSubTile(
      client::CancellationContext context,
      std::shared_ptr<repository::DataRepository> data_repo,
      const PrefetchTilesRequest& request, const std::string& layer_type,
      const repository::SubTilesResult& results,
      const PrefetchTilesResponseCallback& callback);

 private:
  std::shared_ptr<std::atomic_bool> prefetch_provider_busy_;
  std::shared_ptr<repository::ApiRepository> api_repo_;
  std::shared_ptr<repository::CatalogRepository> catalog_repo_;
  std::shared_ptr<repository::DataRepository> data_repo_;
  std::shared_ptr<repository::PrefetchTilesRepository> prefetch_tiles_repo_;
  std::shared_ptr<client::OlpClientSettings> settings_;
  const std::string layer_id_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
