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
#include <string>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/PrefetchTilesRequest.h>
#include <olp/dataservice/read/Types.h>

namespace olp {
namespace dataservice {
namespace read {
class VolatileLayerClientImpl;

// clang-format off
/**
 * @brief Gets data from a volatile layer of the HERE platform.
 *
 * The volatile layer is a key/value store. Values for a given key can change,
 * and only the latest value is retrievable. Therefore, you can only get
 * the latest published data from the volatile layer.
 *
 * Example:
 * @code{.cpp}
 * auto task_scheduler =
 *    olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);
 * auto http_client = olp::client::
 *    OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();
 *
 * olp::authentication::Settings settings{"Your.Key.Id", "Your.Key.Secret"};
 * settings.task_scheduler = task_scheduler;
 * settings.network_request_handler = http_client;
 *
 * olp::client::AuthenticationSettings auth_settings;
 * auth_settings.provider =
 *     olp::authentication::TokenProviderDefault(std::move(settings));
 *
 * auto client_settings = olp::client::OlpClientSettings();
 * client_settings.authentication_settings = auth_settings;
 * client_settings.task_scheduler = std::move(task_scheduler);
 * client_settings.network_request_handler = std::move(http_client);
 *
 * VolatileLayerClient client{"hrn:here:data:::your-catalog-hrn", "your-layer-id", client_settings};
 * auto callback = [](olp::client::ApiResponse<olp::model::Data, olp::client::ApiError> response) {};
 * auto request = DataRequest().WithPartitionId("269");
 * auto token = client.GetData(request, callback);
 * @endcode
 *
 * @see
 * The [volatile
 * layer](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/layers.html#volatile-layers)
 * section in the Data User Guide.
 */
// clang-format on
class DATASERVICE_READ_API VolatileLayerClient final {
 public:
  /**
   * @brief Creates the `VolatileLayerClient` instance.
   *
   * @param catalog The HERE Resource Name (HRN) of the catalog that contains
   * the volatile layer from which you want to get data.
   * @param layer_id The layer ID of the volatile layer from which you want to
   * get data.
   * @param settings The `OlpClientSettings` instance.
   */
  VolatileLayerClient(client::HRN catalog, std::string layer_id,
                      client::OlpClientSettings settings);

  /// Movable, non-copyable
  VolatileLayerClient(const VolatileLayerClient& other) = delete;
  VolatileLayerClient(VolatileLayerClient&& other) noexcept;
  VolatileLayerClient& operator=(const VolatileLayerClient& other) = delete;
  VolatileLayerClient& operator=(VolatileLayerClient&& other) noexcept;

  ~VolatileLayerClient();

  /**
   * @brief Cancels all active and pending requests.
   *
   * @return True if the request is successful; false otherwise.
   */
  bool CancelPendingRequests();

  /**
   * @brief Fetches a list of volatile layer partitions asynchronously.
   *
   * @note If your layer has lots of partitions or uses tile keys as
   * partition IDs, then this operation can fail because of the large amount of
   * data.
   *
   * @param request The `PartitionsRequest` instance that contains a complete
   * set of request parameters.
   * @param callback The `PartitionsResponseCallback` object that is invoked if
   * the list of partitions is available or an error is encountered.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken GetPartitions(PartitionsRequest request,
                                          PartitionsResponseCallback callback);

  /**
   * @brief Fetches a list of volatile layer partitions asynchronously.
   *
   * @note If your layer has lots of partitions or uses tile keys as
   * partition IDs, then this operation can fail because of the large amount of
   * data.
   *
   * @param request The `PartitionsRequest` instance that contains a complete
   * set of request parameters.
   *
   * @return `CancellableFuture` that contains the `PartitionsResponse` instance
   * with data or an error. You can also use `CancellableFuture` to cancel this
   * request.
   */
  olp::client::CancellableFuture<PartitionsResponse> GetPartitions(
      PartitionsRequest request);

  /**
   * @brief Fetches data asynchronously using a partition ID or data handle.
   *
   * If the specified partition or data handle cannot be found in the layer,
   * the callback is invoked with the empty `DataResponse` object (the `nullptr`
   * result and an error). If a partition ID or data handle is not set in
   * the request, the callback is invoked with the following error:
   * `ErrorCode::InvalidRequest`.
   *
   * @param request The `DataRequest` instance that contains a complete set of
   * request parameters.
   * @param callback The `DataResponseCallback` object that is invoked if
   * the `DataResult` object is available or an error is encountered.
   *
   * @return A token that can be used to cancel this request.
   */
  olp::client::CancellationToken GetData(DataRequest request,
                                         DataResponseCallback callback);

  /**
   * @brief Fetches data asynchronously using a partition ID or data handle.
   *
   * If the specified partition or data handle cannot be found in the layer,
   * the callback is invoked with the empty `DataResponse` object (the `nullptr`
   * result and an error). If a partition ID or data handle is not set in
   * the request, the callback is invoked with the following error:
   * `ErrorCode::InvalidRequest`.
   *
   * @param request The `DataRequest` instance that contains a complete set of
   * the request parameters.
   *
   * @return `CancellableFuture` that contains the `DataResponse` instance
   * with data or an error. You can also use `CancellableFuture` to cancel this
   * request.
   */
  olp::client::CancellableFuture<DataResponse> GetData(DataRequest request);

  /**
   * @brief Removes the partition from the mutable disk cache.
   *
   * @param partition_id The partition ID that should be removed.
   *
   * @return True if partition data is removed successfully; false otherwise.
   */
  bool RemoveFromCache(const std::string& partition_id);

  /**
   * @brief Removes the tile from the mutable disk cache.
   *
   * @param tile The tile key that should be removed.
   *
   * @return True if tile data is removed successfully; false otherwise.
   */
  bool RemoveFromCache(const geo::TileKey& tile);

  /**
   * @brief Prefetches a set of tiles asynchronously.
   *
   * This method recursively downloads all tile keys from the `min_level`
   * parameter to the `max_level` parameter of the \c PrefetchTilesRequest
   * object for the given root tiles. If min_level/max_level are the same or
   * default, only tiles listed in \c PrefetchTilesRequest will be downloaded.
   * Only tiles will be downloaded which are not already present in the cache,
   * this helps reduce the network load.
   *
   * @note This method does not guarantee that all tiles are available offline
   * as the cache might overflow, and data might be evicted at any point.
   *
   * @param request The `PrefetchTilesRequest` instance that contains
   * a complete set of request parameters.
   * @param callback The `PrefetchTilesResponseCallback` object that is invoked
   * if the `PrefetchTilesResult` instance is available or an error is
   * encountered.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken PrefetchTiles(
      PrefetchTilesRequest request, PrefetchTilesResponseCallback callback);

  /**
   * @brief Prefetches a set of tiles asynchronously.
   *
   * This method recursively downloads all tile keys from the `min_level`
   * parameter to the `max_level` parameter of the \c PrefetchTilesRequest
   * object for the given root tiles. If min_level/max_level are the same or
   * default, only tiles listed in \c PrefetchTilesRequest will be downloaded.
   * Only tiles will be downloaded which are not already present in the cache,
   * this helps reduce the network load.
   *
   * @note This method does not guarantee that all tiles are available offline
   * as the cache might overflow, and data might be evicted at any point.
   *
   * @param request The `PrefetchTilesRequest` instance that contains
   * a complete set of request parameters.
   *
   * @return `CancellableFuture` that contains the `PrefetchTilesResponse`
   * instance with data or an error. You can also use `CancellableFuture` to
   * cancel this request.
   */
  client::CancellableFuture<PrefetchTilesResponse> PrefetchTiles(
      PrefetchTilesRequest request);

 private:
  std::unique_ptr<VolatileLayerClientImpl> impl_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
