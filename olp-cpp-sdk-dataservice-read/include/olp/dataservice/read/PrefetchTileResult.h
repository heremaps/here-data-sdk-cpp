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

class DATASERVICE_READ_API PrefetchTileNoError {
 public:
  PrefetchTileNoError() = default;
  PrefetchTileNoError(const PrefetchTileNoError& other) = default;
  ~PrefetchTileNoError() = default;
};

/**
 * @brif Class representing the result of pre-fetch operation.
 * It will contain either a successful result represented as \c geo::TileKey
 * object or will contain the failure error.
 */
class DATASERVICE_READ_API PrefetchTileResult
    : public client::ApiResponse<PrefetchTileNoError, client::ApiError> {
 public:
  PrefetchTileResult()
      : client::ApiResponse<PrefetchTileNoError, client::ApiError>() {}

  /**
   * @brief PrefetchTileResult Constructor for a successfully executed request.
   */
  PrefetchTileResult(const geo::TileKey& tile,
                     const PrefetchTileNoError& result)
      : client::ApiResponse<PrefetchTileNoError, client::ApiError>(result),
        tile_key_(tile) {}

  /**
   * @brief PrefetchTileResult Constructor if request unsuccessfully executed
   */
  PrefetchTileResult(const geo::TileKey& tile, const client::ApiError& error)
      : client::ApiResponse<PrefetchTileNoError, client::ApiError>(error),
        tile_key_(tile) {}

  /**
   * @brief PrefetchTileResult Copy constructor.
   */
  PrefetchTileResult(const PrefetchTileResult& r)
      : client::ApiResponse<PrefetchTileNoError, client::ApiError>(r),
        tile_key_(r.tile_key_) {}

  /**
   * @brief ApiResponse Constructor if request unsuccessfully executed
   */
  PrefetchTileResult(const client::ApiError& error)
      : client::ApiResponse<PrefetchTileNoError, client::ApiError>(error) {}

 public:
  /// The pre-fetched tile key.
  geo::TileKey tile_key_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
