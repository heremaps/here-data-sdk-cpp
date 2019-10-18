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

#include <memory>
#include <vector>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/PrefetchTileResult.h>
#include <olp/dataservice/read/PrefetchTilesRequest.h>
#include <olp/dataservice/read/model/Data.h>
#include <olp/dataservice/read/model/Partitions.h>

namespace olp {
namespace dataservice {
namespace read {
class VersionedLayerClientImpl;

/**
 * @brief The VersionedLayerClient aimed to acquire data from OLP services.
 */
class DATASERVICE_READ_API VersionedLayerClient final {
 public:
  /// DataResult alias
  using DataResult = model::Data;
  /// CallbackResponse alias
  using CallbackResponse = client::ApiResponse<DataResult, client::ApiError>;
  /// Callback alias
  using Callback = std::function<void(CallbackResponse response)>;

  using PartitionsResult = model::Partitions;
  using PartitionsResponse =
      client::ApiResponse<PartitionsResult, client::ApiError>;
  using PartitionsCallback = std::function<void(PartitionsResponse response)>;

  /// PrefetchTileResult alias
  using PrefetchTilesResult = std::vector<std::shared_ptr<PrefetchTileResult>>;
  /// Alias for the response to the \c PrefetchTileRequest
  using PrefetchTilesResponse =
      client::ApiResponse<PrefetchTilesResult, client::ApiError>;
  /// Alias for the callback  to the results of PrefetchTilesResponse
  using PrefetchTilesResponseCallback =
      std::function<void(const PrefetchTilesResponse& response)>;

  /**
   * @brief VersionedLayerClient constructor
   * @param catalog this versioned layer client uses during requests.
   * @param layer_id this versioned layer client uses during requests.
   * @param settings the settings used to control behaviour of the client
   * instance.
   */
  VersionedLayerClient(olp::client::HRN catalog, std::string layer_id,
                       olp::client::OlpClientSettings client_settings);

  ~VersionedLayerClient();

  /**
   * @brief Fetches data for a partition or data handle asynchronously. If the
   * specified partition or data handle cannot be found in the layer, the
   * callback will be invoked with an empty DataResponse (nullptr for result and
   * error). If neither Partition Id or Data Handle were set in the request, the
   * callback will be invoked with an error with ErrorCode::InvalidRequest.
   * @param data_request contains the complete set of request parameters.
   * \note \c DataRequest's GetLayerId value will be ignored and the parameter
   * from the constructor will be used instead.
   * @param callback will be invoked once the DataResult is available, or an
   * error is encountered.
   * @return A token that can be used to cancel this request.
   */
  olp::client::CancellationToken GetData(DataRequest data_request,
                                         Callback callback);

  /**
   * @brief fetches a list partitions for given generic layer asynchronously.
   * @param request contains the complete set of request parameters.
   * \note \c PartitionsRequest's GetLayerId value will be ignored and the
   * parameter from the constructor will be used instead.
   * @param callback will be invoked once the list of partitions is available,
   * or an error is encountered.
   * @return A token that can be used to cancel this request
   */
  client::CancellationToken GetPartitions(PartitionsRequest partitions_request,
                                          PartitionsCallback callback) const;

  /**
   * @brief Pre-fetches a set of tiles asychronously.
   *
   * This method recursively downloads all tilekeys from minLevel to maxLevel
   * specified in the \c PrefetchTilesRequest's properties. This will help to
   * reduce the network load by using the pre-fetched tiles' data from cache.
   *
   * \note - this does not guarantee that all tiles are available offline, as
   * the cache might overflow and data might be evicted at any point.
   *
   * @param request contains the complete set of request parameters.
   * \note The \c PrefetchTilesRequest's GetLayerId value will be ignored and
   * the parameter from constructore will be used instead.
   * @param callback will be invoked once the \c PrefetchTilesResult is
   * available, or an error is encountered.
   * @return A token that can be used to cancel this request.
   */
  olp::client::CancellationToken PrefetchTiles(
      PrefetchTilesRequest request, PrefetchTilesResponseCallback callback);

 private:
  std::unique_ptr<VersionedLayerClientImpl> impl_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
