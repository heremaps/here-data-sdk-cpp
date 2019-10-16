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

#include <sstream>
#include <string>
#include <vector>

#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/porting/deprecated.h>
#include <boost/optional.hpp>
#include "DataServiceReadApi.h"

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief The PrefetchTilesRequest class encapsulates the fields required to
 * prefetch the specified layers, tiles and levels.
 *
 * Tilekeys can be on any level. Tilekeys below maxLevel will have the ancestors
 * fetched from minLevel to maxLevel. Children of tilekeys above minLevel will
 * be downloaded from minLevel to maxLevel. Tilekeys above maxLevel will be
 * recursively downloaded down to maxLevel.
 */
class DATASERVICE_READ_API PrefetchTilesRequest final {
 public:
  /**
   * @brief GetLayerId gets the request's layer id.
   * @return the layer id.
   * @deprecated Will be removed in 1.0
   */
  OLP_SDK_DEPRECATED("Will be removed in 1.0")
  inline const std::string& GetLayerId() const { return layer_id_; }

  /**
   * @brief WithLayerId sets the layer id of the request.
   * @param layerId the layer id the requested tiles belong to.
   * @return a reference to the updated PrefetchTilesRequest.
   * @deprecated Will be removed in 1.0
   */
  OLP_SDK_DEPRECATED("Will be removed in 1.0")
  inline PrefetchTilesRequest& WithLayerId(const std::string& layerId) {
    layer_id_ = layerId;
    return *this;
  }

  /**
   * @brief WithLayerId sets the layer id of the request.
   * @param layerId the layer id the requested tiles belong to.
   * @return a reference to the updated PrefetchTilesRequest.
   * @deprecated Will be removed in 1.0
   */
  OLP_SDK_DEPRECATED("Will be removed in 1.0")
  inline PrefetchTilesRequest& WithLayerId(std::string&& layerId) {
    layer_id_ = std::move(layerId);
    return *this;
  }

  /**
   * @brief GetPartitionId gets the request's Partition Id.
   * @return the partition id.
   */
  inline const std::vector<geo::TileKey>& GetTileKeys() const {
    return tile_keys_;
  }

  /**
   * @brief WithTileKeys sets the request's tile keys. If the tile key
   * cannot be found in the layer, the callback will come back with an empty
   * response (null for data and error)
   * @param partitionId the Partition Id.
   * @return a reference to the updated PrefetchTilesRequest.
   */
  inline PrefetchTilesRequest& WithTileKeys(
      std::vector<geo::TileKey> tile_keys) {
    tile_keys_ = tile_keys;
    return *this;
  }

  /**
   * @brief WithTileKeys sets the request's tile keys. If the tile key
   * cannot be found in the layer, the callback will come back with an empty
   * response (null for data and error).
   * @param tile_keys the Tile Keys.
   * @return a reference to the updated PrefetchTilesRequest.
   */
  inline PrefetchTilesRequest& WithTileKeys(
      std::vector<geo::TileKey>&& tile_keys) {
    tile_keys_ = std::move(tile_keys);
    return *this;
  }

  inline unsigned int GetMinLevel() const { return min_level_; }
  inline PrefetchTilesRequest& WithMinLevel(unsigned int min_level) {
    min_level_ = min_level;
    return *this;
  }

  inline unsigned int GetMaxLevel() const { return max_level_; }
  inline PrefetchTilesRequest& WithMaxLevel(unsigned int max_level) {
    max_level_ = max_level;
    return *this;
  }

  /**
   * @brief BillingTag is an optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @return the billing tag, or boost::none if not set.
   */
  inline const boost::optional<std::string>& GetBillingTag() const {
    return billing_tag_;
  }

  /**
   * @brief WithBillingTag sets the billing tag. See ::GetBillingTag() for usage
   * and format.
   * @param billingTag a string or boost::none
   * @return a reference to the updated CatalogRequest
   */
  inline PrefetchTilesRequest& WithBillingTag(
      boost::optional<std::string> billingTag) {
    billing_tag_ = billingTag;
    return *this;
  }

  /**
   * @brief WithBillingTag sets the billing tag. See ::GetBillingTag() for usage
   * and format.
   * @param billingTag a string or boost::none
   * @return a reference to the updated CatalogRequest
   */
  inline PrefetchTilesRequest& WithBillingTag(std::string&& billingTag) {
    billing_tag_ = std::move(billingTag);
    return *this;
  }

  /**
   * @brief Creates readable format for the request.
   * @param none
   * @return string representation of the request
   */
  inline std::string CreateKey() const {
    std::stringstream out;
    out << GetLayerId();

    out << "[" << GetMinLevel() << "/" << GetMaxLevel() << "]";

    out << "(" << GetTileKeys().size() << ")";

    if (GetBillingTag()) {
      out << "$" << GetBillingTag().get();
    }

    return out.str();
  }

 private:
  std::string layer_id_;
  std::vector<geo::TileKey> tile_keys_;
  unsigned int min_level_{0};
  unsigned int max_level_{0};
  boost::optional<std::string> billing_tag_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
