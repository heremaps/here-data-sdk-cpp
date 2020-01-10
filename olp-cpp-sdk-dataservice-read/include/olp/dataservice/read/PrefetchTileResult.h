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

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/dataservice/read/DataServiceReadApi.h>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief A helper class for the 'ApiResponse` class.
 */
class DATASERVICE_READ_API PrefetchTileNoError {
 public:
  PrefetchTileNoError() = default;
  PrefetchTileNoError(const PrefetchTileNoError& other) = default;
  ~PrefetchTileNoError() = default;
};

/**
 * @brief Represents the result of a prefetch operation.
 *
 * If successful, contains the \c geo::TileKey object; otherwise, contains
 * the failure error.
 */
class DATASERVICE_READ_API PrefetchTileResult
    : public client::ApiResponse<PrefetchTileNoError, client::ApiError> {
 public:
  /**
   * @brief A parent type of the current `PrefetchTileResult` class.
   */
  using base_type = client::ApiResponse<PrefetchTileNoError, client::ApiError>;

  PrefetchTileResult() : base_type() {}

  /**
   * @brief Creates the `PrefetchTileResult` instance if the corresponding
   * response was successful.
   *
   * @param tile The `TileKey` instance that addresses a tile in a quadtree.
   * @param result The `PrefetchTileNoError` instance.
   */
  PrefetchTileResult(const geo::TileKey& tile,
                     const PrefetchTileNoError& result)
      : base_type(result), tile_key_(tile) {}

  /**
   * @brief Creates the `PrefetchTileResult` instance if the corresponding
   * response was not successful.
   *
   * @param tile The `TileKey` instance that addresses a tile in a quadtree.
   * @param error The `ApiError` instance that contains information about the
   * error.
   */
  PrefetchTileResult(const geo::TileKey& tile, const client::ApiError& error)
      : base_type(error), tile_key_(tile) {}

  /**
   * @brief Creates the `PrefetchTileResult` instance that is a copy of the `r`
   * parameter.
   *
   * @param r The `PrefetchTileResult` instance that contains a tile key and
   * error information.
   */
  PrefetchTileResult(const PrefetchTileResult& r)
      : base_type(r), tile_key_(r.tile_key_) {}

  /**
   * @brief Creates the `ApiResponse` instance if the corresponding response was
   * not successful.
   *
   * @param error The `ApiError` instance that contains information about
   * the error.
   */
  PrefetchTileResult(const client::ApiError& error) : base_type(error) {}

 public:
  /**
   * @brief The `TileKey` instance that addresses a tile in a quadtree.
   *
   * @param tile_key_ The prefetched tile key.
   */
  geo::TileKey tile_key_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
