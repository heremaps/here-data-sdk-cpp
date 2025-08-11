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

#pragma once

#include <sstream>
#include <string>
#include <utility>

#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/porting/optional.h>
#include <olp/core/thread/TaskScheduler.h>
#include <olp/dataservice/read/FetchOptions.h>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Encapsulates the fields required to request tile for the given
 * key.
 *
 * You should specify tile key. Additionaly offset and depth could be set. If
 * not set used default values, for offset default value is 2, for depth - 4.
 */
class DATASERVICE_READ_API TileRequest final {
 public:
  /**
   * @brief Gets the billing tag to group billing records together.
   *
   * The billing tag is an optional free-form tag that is used for grouping
   * billing records together. If supplied, it must be 4–16 characters
   * long and contain only alphanumeric ASCII characters [A-Za-z0-9].
   *
   * @return The `BillingTag` string or `olp::porting::none` if the billing tag
   * is not set.
   */
  const porting::optional<std::string>& GetBillingTag() const {
    return billing_tag_;
  }

  /**
   * @brief Sets the billing tag for the request.
   *
   * @see `GetBillingTag()` for information on usage and format.
   *
   * @param tag The `BillingTag` string or `olp::porting::none`.
   *
   * @return A reference to the updated `TileRequest` instance.
   */
  template <class T = porting::optional<std::string>>
  TileRequest& WithBillingTag(T&& tag) {
    billing_tag_ = std::forward<T>(tag);
    return *this;
  }

  /**
   * @brief Sets the tile key for the request.
   *
   * @param tile_key Tile key value.
   *
   * @return A reference to the updated `TileRequest` instance.
   */
  TileRequest& WithTileKey(geo::TileKey tile_key) {
    tile_key_ = std::move(tile_key);
    return *this;
  }

  /**
   * @brief Gets a tile key value.
   *
   * @return The tile key or `olp::porting::none` if tile_key is not set.
   */
  const geo::TileKey& GetTileKey() const { return tile_key_; }

  /**
   * @brief Gets the fetch option that controls how requests are handled.
   *
   * The default option is `OnlineIfNotFound` that queries the network if
   * the requested resource is not in the cache.
   *
   * @return The fetch option.
   */
  FetchOptions GetFetchOption() const { return fetch_option_; }

  /**
   * @brief Sets the fetch option that you can use to set the source from
   * which data should be fetched.
   *
   * @see `GetFetchOption()` for information on usage and format.
   *
   * @param fetch_option The `FetchOption` enum.
   *
   * @return A reference to the updated `TileRequest` instance.
   */
  TileRequest& WithFetchOption(FetchOptions fetch_option) {
    fetch_option_ = fetch_option;
    return *this;
  }

  /**
   * @brief Gets the request priority.
   *
   * The default priority is `Priority::NORMAL`.
   *
   * @return The request priority.
   */
  uint32_t GetPriority() const { return priority_; }

  /**
   * @brief Sets the priority of the request.
   *
   * @param priority The priority of the request.
   *
   * @return A reference to the updated `TileRequest` instance.
   */
  TileRequest& WithPriority(uint32_t priority) {
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
  std::string CreateKey(const std::string& layer_id) const {
    std::stringstream out;
    out << layer_id << "[" << GetTileKey().ToHereTile() << "]";
    if (GetBillingTag()) {
      out << "$" << *GetBillingTag();
    }
    out << "^" << GetFetchOption();
    return out.str();
  }

 private:
  porting::optional<std::string> billing_tag_;
  geo::TileKey tile_key_;
  FetchOptions fetch_option_{OnlineIfNotFound};
  uint32_t priority_{thread::NORMAL};
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
