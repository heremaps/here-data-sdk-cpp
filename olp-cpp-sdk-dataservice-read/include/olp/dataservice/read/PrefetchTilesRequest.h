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

#pragma once

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/porting/deprecated.h>
#include <olp/core/thread/TaskScheduler.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <boost/optional.hpp>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Encapsulates the fields required to prefetch the specified layers,
 * tiles, and levels.
 *
 * Tile keys can be at any level. Tile keys below the maximum tile level have
 * the ancestors fetched from the minimum tile level. The children of the tile
 * keys above the minimum tile level are downloaded from the minimum to maximum
 * tile level. The tile keys above the maximum tile level are recursively
 * downloaded down to the maximum tile level.
 */
class DATASERVICE_READ_API PrefetchTilesRequest final {
 public:
  /**
   * @brief Get the vector of the root tile keys.
   *
   * @return The vector with the tile keys.
   */
  inline const std::vector<geo::TileKey>& GetTileKeys() const {
    return tile_keys_;
  }

  /**
   * @brief Sets the vector of the root tile keys for the request.
   *
   * @param tile_keys The vector with the root tile keys of the prefetch.
   *
   * @return A reference to the updated `PrefetchTilesRequest` instance.
   */
  inline PrefetchTilesRequest& WithTileKeys(
      std::vector<geo::TileKey> tile_keys) {
    tile_keys_ = std::move(tile_keys);
    return *this;
  }

  /**
   * @brief Gets the minimum tiles level to prefetch.
   *
   * @return The minimum tile level.
   */
  inline unsigned int GetMinLevel() const { return min_level_; }

  /**
   * @brief Sets the minimum tiles level for the request.
   *
   * @param min_level The minimum tiles level to prefetch.
   *
   * @return A reference to the updated `PrefetchTilesRequest` instance.
   */
  inline PrefetchTilesRequest& WithMinLevel(unsigned int min_level) {
    min_level_ = min_level;
    return *this;
  }

  /**
   * @brief Gets the maximum tiles level to prefetch.
   *
   * @return The maximum tile level.
   */
  inline unsigned int GetMaxLevel() const { return max_level_; }

  /**
   * @brief Sets the maximum tile level for the request.
   *
   * @param max_level The maximum tile level to prefetch.
   *
   * @return A reference to the updated `PrefetchTilesRequest` instance.
   */
  inline PrefetchTilesRequest& WithMaxLevel(unsigned int max_level) {
    max_level_ = max_level;
    return *this;
  }

  /**
   * @brief Gets the billing tag to group billing records together.
   *
   * The billing tag is an optional free-form tag that is used for grouping
   * billing records together. If supplied, it must be 4–16 characters
   * long and contain only alphanumeric ASCII characters [A-Za-z0-9].
   *
   * @return The `BillingTag` string or `boost::none` if the billing tag is not
   * set.
   */
  inline const boost::optional<std::string>& GetBillingTag() const {
    return billing_tag_;
  }

  /**
   * @brief Sets the billing tag for the request.
   *
   * @see `GetBillingTag()` for information on usage and format.
   *
   * @param tag The `BillingTag` string or `boost::none`.
   *
   * @return A reference to the updated `PrefetchTilesRequest` instance.
   */
  inline PrefetchTilesRequest& WithBillingTag(
      boost::optional<std::string> tag) {
    billing_tag_ = std::move(tag);
    return *this;
  }

  /**
   * @brief Sets the billing tag for the request.
   *
   * @see `GetBillingTag()` for information on usage and format.
   *
   * @param tag The rvalue reference to the `BillingTag` string or
   * `boost::none`.
   *
   * @return A reference to the updated `PrefetchTilesRequest` instance.
   */
  inline PrefetchTilesRequest& WithBillingTag(std::string&& tag) {
    billing_tag_ = std::move(tag);
    return *this;
  }

  /**
   * @brief Changes the prefetch behavior when prefetching a list of tiles.
   *
   * In case a tile does not exist, the prefetch algorithm searches for the
   * nearest parent and prefetches it.
   *
   * @param data_aggregation_enabled The boolean parameter that enables or
   * disables the aggregation.
   *
   * @note Experimental. API may change.
   * 
   * @return A reference to the updated `PrefetchTilesRequest` instance.
   */
  inline PrefetchTilesRequest& WithDataAggregationEnabled(
      bool data_aggregation_enabled) {
    data_aggregation_enabled_ = data_aggregation_enabled;
    return *this;
  }

  /**
   * @brief Gets the data aggregation flag.
   *
   * @note Experimental. API may change.
   *
   * @return The data aggregation flag as a boolean value.
   */
  inline bool GetDataAggregationEnabled() const {
    return data_aggregation_enabled_;
  }

  /**
   * @brief Gets the request priority.
   *
   * The default priority is `Priority::LOW`.
   *
   * @return The request priority.
   */
  inline uint32_t GetPriority() const { return priority_; }

  /**
   * @brief Sets the priority of the prefetch request.
   *
   * @param priority The priority of the request.
   *
   * @return A reference to the updated `PrefetchTileRequest` instance.
   */
  inline PrefetchTilesRequest& WithPriority(uint32_t priority) {
    priority_ = priority;
    return *this;
  }

  /**
   * @brief Creates a readable format for the request.
   *
   * @param layer_id The ID of the layer that is used for the request.
   *
   * @return A string representation of the request.
   */
  inline std::string CreateKey(const std::string& layer_id) const {
    std::stringstream out;
    out << layer_id << "[" << GetMinLevel() << "/" << GetMaxLevel() << "]"
        << "(" << GetTileKeys().size() << ")";
    if (GetBillingTag()) {
      out << "$" << GetBillingTag().get();
    }
    return out.str();
  }

 private:
  std::string layer_id_;
  std::vector<geo::TileKey> tile_keys_;
  unsigned int min_level_{geo::TileKey::LevelCount};
  unsigned int max_level_{geo::TileKey::LevelCount};
  boost::optional<std::string> billing_tag_;
  bool data_aggregation_enabled_{false};
  uint32_t priority_{thread::LOW};
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
