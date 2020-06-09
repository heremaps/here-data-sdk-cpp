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

#include <utility>

#include <olp/core/geo/tiling/TileKey.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/model/Data.h>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Represents the result of a aggregated data operation.
 */
class DATASERVICE_READ_API AggregatedDataResult {
 public:
  AggregatedDataResult() = default;

  virtual ~AggregatedDataResult() = default;

  /**
   * @brief Sets the `TileKey` instance of this content.
   *
   * @param tile_key The fetched tile key.
   */
  void SetTile(const geo::TileKey& tile_key) { tile_key_ = tile_key; }

  /**
   * @brief Gets the requested TileKey or any of it's closest ancestor which
   * contains the related data.
   *
   * @return The fetched tile key.
   */
  const geo::TileKey& GetTile() const { return tile_key_; }

  /**
   * @brief Sets the data of the tile.
   *
   * @param data of the prefetched tile.
   */
  void SetData(model::Data data) { data_ = std::move(data); }

  /**
   * @brief Gets the data of the tile.
   *
   * @return The prefetched data of the tile.
   */
  const model::Data& GetData() const { return data_; }

  /**
   * @brief Moves the data.
   *
   * @return The forwarding reference to the data.
   */
  model::Data&& MoveData() { return std::move(data_); }

 private:
  geo::TileKey tile_key_;
  model::Data data_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
