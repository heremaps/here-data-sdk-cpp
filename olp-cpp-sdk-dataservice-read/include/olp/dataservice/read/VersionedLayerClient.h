/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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
#include <vector>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/PrefetchPartitionsRequest.h>
#include <olp/dataservice/read/PrefetchTileResult.h>
#include <olp/dataservice/read/PrefetchTilesRequest.h>
#include <olp/dataservice/read/TileRequest.h>
#include <olp/dataservice/read/Types.h>
#include <boost/optional.hpp>

namespace olp {
namespace dataservice {
namespace read {
class VersionedLayerClientImpl;

// clang-format off
/**
 * @brief Gets data from a versioned layer of the HERE platform.
 *
 * The versioned layer stores slowly-changing data that must remain logically
 * consistent with other layers in a catalog. You can request any data version
 * from the versioned layer.
 * When you request a particular version of data from the versioned layer,
 * the partition you receive in the response may have a lower version number
 * than you requested. The version of a layer or partition represents the
 * catalog version in which the layer or partition was last updated.
 *
 * @note If the catalog version is not specified, the version is set upon the
 * first request made, based on this rules which apply for each specific FetchOptions:
 * - OnlineOnly - version is resolved from online.
 * - CacheOnly - version is resolved from cache.
 * - OnlineIfNotFound - retrieve from online first, then checks the cache in
 * case of error. Update cache version if online version is higher then the
 * cache version.
 *
 * An example with the catalog version provided that saves one network request:
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
 * VersionedLayerClient client{"hrn:here:data:::your-catalog-hrn", "your-layer-id", "catalog-version", client_settings};
 * auto callback = [](olp::client::ApiResponse<olp::model::Data, olp::client::ApiError> response) {};
 * auto request = DataRequest().WithPartitionId("269");
 * auto token = client.GetData(request, callback);
 * @endcode
 *
 * @see
 * The [versioned
 * layer](https://developer.here.com/documentation/data-user-guide/portal/layers/layers.html#versioned-layers)
 * section in the Data User Guide.
 */
// clang-format on
class DATASERVICE_READ_API VersionedLayerClient final {
 public:
  /**
   * @brief Creates the `VersionedLayerClient` instance with the specified
   * catalog version.
   *
   * The instance of this client is locked to the specified catalog version
   * passed to the constructor and can't be changed. This way we assure data
   * consistency. Keep in mind that catalog version provided with requests like
   * \ref DataRequest, \ref PartitionsRequest, and \ref PrefetchTilesRequest
   * will be ignored.
   *
   * @note If you didn't specify the catalog version, the last available version
   * is requested once and used for the entire lifetime of this instance.
   *
   * @param catalog The HERE Resource Name (HRN) of the catalog that contains
   * the versioned layer from which you want to get data.
   * @param layer_id The layer ID of the versioned layer from which you want to
   * get data.
   * @param catalog_version The version of the catalog from which you want
   * to get data. If no version is specified, the last available version is
   * used instead.
   * @param settings The `OlpClientSettings` instance.
   *
   * @return The `VersionedLayerClient` instance with the specified catalog
   * version.
   */
  VersionedLayerClient(client::HRN catalog, std::string layer_id,
                       boost::optional<int64_t> catalog_version,
                       client::OlpClientSettings settings);

  /// A copy constructor.
  VersionedLayerClient(const VersionedLayerClient& other) = delete;

  /// A default move constructor.
  VersionedLayerClient(VersionedLayerClient&& other) noexcept;

  /// A copy assignment operator.
  VersionedLayerClient& operator=(const VersionedLayerClient& other) = delete;

  /// A move assignment operator.
  VersionedLayerClient& operator=(VersionedLayerClient&& other) noexcept;

  ~VersionedLayerClient();

  /**
   * @brief Cancels all active and pending requests.
   *
   * @return True if the request is successful; false otherwise.
   */
  bool CancelPendingRequests();

  /**
   * @brief Fetches data asynchronously using a partition ID or data handle.
   *
   * If the specified partition ID or data handle cannot be found in the layer,
   * the callback is invoked with the empty `DataResponse` object (the `nullptr`
   * result and an error). If a partition ID or data handle is not set in
   * the request, the callback is invoked with the following error:
   * `ErrorCode::InvalidRequest`. If the version is not specified, an additional
   * request to the HERE platform is created to retrieve the latest available
   * partition version.
   *
   * @param data_request The `DataRequest` instance that contains a complete set
   * of request parameters.
   * @note CacheWithUpdate fetch option is not supported.
   * @param callback The `DataResponseCallback` object that is invoked if
   * the `DataResult` object is available or an error is encountered.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken GetData(DataRequest data_request,
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
   * @param data_request The `DataRequest` instance that contains a complete set
   * of request parameters.
   * @note CacheWithUpdate fetch option is not supported.
   *
   * @return `CancellableFuture` that contains the `DataResponse` instance
   * or an error. You can also use `CancellableFuture` to cancel this request.
   */
  client::CancellableFuture<DataResponse> GetData(DataRequest data_request);

  /**
   * @brief Fetches data asynchronously using a TileKey.
   *
   * If the specified TileKey cannot be found in the layer, the callback is
   * invoked with the empty `DataResponse` object (the `nullptr` result and an
   * error). Version for request used from VersionedLayerClient constructor
   * parameter. If no version is specified, the last available version is used
   * instead. GetData(TileRequest) method optimizes the metadata query by
   * requesting a QuadTree with depth 4 and store all subquads in cache. This
   * way all further GetData(TileRequest) request that are contained within this
   * QuadTree will profit from the already cached metadata.
   * @note Calling this method only makes sense if you have a persistent cache
   * connected.
   *
   * @param request The `TileRequest` instance that contains a complete set
   * of request parameters.
   * @note CacheWithUpdate fetch option is not supported.
   * @param callback The `DataResponseCallback` object that is invoked if
   * the `DataResult` object is available or an error is encountered.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken GetData(TileRequest request,
                                    DataResponseCallback callback);
  /**
   * @brief Fetches data asynchronously using a TileKey.
   *
   * If the specified TileKey cannot be found in the layer, the callback is
   * invoked with the empty `DataResponse` object (the `nullptr` result and an
   * error). Version for request used from VersionedLayerClient constructor
   * parameter. If no version is specified, the last available version is used
   * instead.GetData(TileRequest) method optimizes the metadata query by
   * requesting a QuadTree with depth 4 and store all subquads in cache. This
   * way all further GetData(TileRequest) request that are contained within this
   * QuadTree will profit from the already cached metadata.
   * @note Calling this method only makes sense if you have a persistent cache
   * connected.
   *
   * @param request The `TileRequest` instance that contains a complete set
   * of request parameters.
   * @note CacheWithUpdate fetch option is not supported.
   *
   * @return `CancellableFuture` that contains the `DataResponse` instance
   * or an error. You can also use `CancellableFuture` to cancel this request.
   */
  client::CancellableFuture<DataResponse> GetData(TileRequest request);

  /**
   * @brief Fetches data of a tile or its closest ancestor.
   *
   * @param request The `TileRequest` instance that contains a complete set
   * of request parameters.
   * @note CacheWithUpdate fetch option is not supported.
   * @param callback The `AggregatedDataResponseCallback` object that is invoked
   * if the `AggregatedDataResult` object is available or an error is
   * encountered.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken GetAggregatedData(
      TileRequest request, AggregatedDataResponseCallback callback);

  /**
   * @brief Fetches data of a tile or its closest ancestor.
   *
   * @param request The `TileRequest` instance that contains a complete set
   * of request parameters.
   * @note CacheWithUpdate fetch option is not supported.
   *
   * @return `CancellableFuture` that contains the `AggregatedDataResponse`
   * instance or an error. You can also use `CancellableFuture` to cancel this
   * request.
   */
  client::CancellableFuture<AggregatedDataResponse> GetAggregatedData(
      TileRequest request);

  /**
   * @brief Fetches a list of partitions of the given generic layer
   * asynchronously.
   *
   * @note If your layer has lots of partitions or uses tile keys as
   * partition IDs, then this operation can fail because of the large amount of
   * data.
   *
   * @param partitions_request The `PartitionsRequest` instance that contains
   * a complete set of request parameters.
   * @note CacheWithUpdate fetch option is not supported.
   * @param callback The `PartitionsResponseCallback` object that is invoked if
   * the list of partitions is available or an error is encountered.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken GetPartitions(PartitionsRequest partitions_request,
                                          PartitionsResponseCallback callback);

  /**
   * @brief Fetches a list of partitions of the given generic layer
   * asynchronously. Client does not cache the partitions, instead every
   * partition is passed to the provided callback.
   *
   * @note API is considered experimental and a subject to change.
   *
   * @param partitions_request The `PartitionsRequest` instance that contains
   * a complete set of request parameters.
   * @note Fetch option and partition list are not supported.
   * @param partition_stream_callback The `PartitionsStreamCallback` that
   * receives every fetched partition.
   * @param callback The `CallbackNoResult` object that is invoked when
   * operation is complete or an error is encountered.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken StreamLayerPartitions(
      PartitionsRequest partitions_request,
      PartitionsStreamCallback partition_stream_callback,
      CallbackNoResult callback);

  /**
   * @brief Fetches a list of partitions of the given generic layer
   * asynchronously.
   *
   * @note If your layer has lots of partitions or uses tile keys as
   * partition IDs, then this operation can fail because of the large amount of
   * data.
   *
   * @param partitions_request The `PartitionsRequest` instance that contains
   * a complete set of request parameters.
   * @note CacheWithUpdate fetch option is not supported.
   *
   * @return `CancellableFuture` that contains the `PartitionsResponse` instance
   * with data or an error. You can also use `CancellableFuture` to cancel this
   * request.
   */
  client::CancellableFuture<PartitionsResponse> GetPartitions(
      PartitionsRequest partitions_request);

  /**
   * @brief Fetches a list of partitions including data size, checksum and crc
   * asynchronously.
   *
   * @note CacheWithUpdate fetch option is not supported.
   * @note If OnlineIfNotFound fetch option is used and the cached data does not
   * contain data size, checksum or crc, a new network request is triggered to
   * download required data and update the cache record.
   *
   * @param tile_request The `TileRequest` instance that contains
   * a complete set of request parameters.
   * @param callback The `PartitionsResponseCallback` object that is invoked if
   * the list of partitions is available or an error is encountered.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken QuadTreeIndex(TileRequest tile_request,
                                          PartitionsResponseCallback callback);

  /**
   * @brief Prefetches a set of tiles asynchronously.
   *
   * This method recursively downloads all tile keys from the `min_level`
   * parameter to the `max_level` parameter of the \c PrefetchTilesRequest
   * object for the given root tiles. If min_level/max_level are default, only
   * tiles listed in \c PrefetchTilesRequest will be downloaded. Only tiles will
   * be downloaded which are not already present in the cache, this helps reduce
   * the network load.
   *
   * @note This method does not guarantee that all tiles are available offline
   * as the cache might overflow, and data might be evicted at any point.
   * Use GetData(TileRequest) or GetAggregatedData(TileRequest) to retrieve
   * tiles loaded by PrefetchTiles.
   *
   * @param request The `PrefetchTilesRequest` instance that contains
   * a complete set of request parameters.
   * @param callback The `PrefetchTilesResponseCallback` object that is invoked
   * if the `PrefetchTilesResult` instance is available or an error is
   * encountered.
   * @param status_callback The `PrefetchStatusCallback` object that is invoked
   * every time a tile is fetched.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken PrefetchTiles(
      PrefetchTilesRequest request, PrefetchTilesResponseCallback callback,
      PrefetchStatusCallback status_callback = nullptr);

  /**
   * @brief Prefetches a set of tiles asynchronously.
   *
   * This method recursively downloads all tile keys from the `min_level`
   * parameter to the `max_level` parameter of the \c PrefetchTilesRequest
   * object for the given root tiles. If min_level/max_level are default, only
   * tiles listed in \c PrefetchTilesRequest will be downloaded. Only tiles will
   * be downloaded which are not already present in the cache, this helps reduce
   * the network load.
   *
   * @note This method does not guarantee that all tiles are available offline
   * as the cache might overflow, and data might be evicted at any point.
   * Use GetData(TileRequest) or GetAggregatedData(TileRequest) to retrieve
   * tiles loaded by PrefetchTiles.
   *
   * @param request The `PrefetchTilesRequest` instance that contains
   * a complete set of request parameters.
   * @param status_callback The `PrefetchStatusCallback` object that is invoked
   * every time a tile is fetched.
   *
   * @return `CancellableFuture` that contains the `PrefetchTilesResponse`
   * instance with data or an error. You can also use `CancellableFuture` to
   * cancel this request.
   */
  client::CancellableFuture<PrefetchTilesResponse> PrefetchTiles(
      PrefetchTilesRequest request,
      PrefetchStatusCallback status_callback = nullptr);

  /**
   * @brief Prefetches a set of partitions asynchronously.
   *
   * This method downloads all partitions listed in
   * `PrefetchPartitionsRequest`. Only partitions that are not already present
   * in the cache are downloaded. It helps reduce the network load.
   *
   * @note This method does not guarantee that all partitions are available
   * offline as the cache might overflow, and data might be evicted at any
   * point. Use `GetData(DataRequest)` to retrieve partitions loaded by
   * `PrefetchPartitions`.
   *
   * @param request The `PrefetchPartitionsRequest` instance that contains
   * a complete set of request parameters.
   * @param callback The `PrefetchPartitionsResponseCallback` object that is
   * invoked if the `PrefetchPartitionsResult` instance is available or an error
   * occurs.
   * @param status_callback The `PrefetchPartitionsStatusCallback` object that
   * is invoked every time a partition is fetched.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken PrefetchPartitions(
      PrefetchPartitionsRequest request,
      PrefetchPartitionsResponseCallback callback,
      PrefetchPartitionsStatusCallback status_callback = nullptr);

  /**
   * @brief Prefetches a set of partitions asynchronously.
   *
   * This method downloads all partitions listed in
   * `PrefetchPartitionsRequest`. Only partitions that are not already present
   * in the cache are downloaded. It helps reduce the network load.
   *
   * @note This method does not guarantee that all partitions are available
   * offline as the cache might overflow, and data might be evicted at any
   * point. Use `GetData(DataRequest)` to retrieve partitions loaded by
   * `PrefetchPartitions`.
   *
   * @param request The `PrefetchPartitionsRequest` instance that contains
   * a complete set of request parameters.
   * @param status_callback The `PrefetchPartitionsStatusCallback` object that
   * is invoked every time a partition is fetched.
   *
   * @return `CancellableFuture` that contains the `PrefetchPartitionsResponse`
   * instance with data or an error. You can also use `CancellableFuture` to
   * cancel this request.
   */
  client::CancellableFuture<PrefetchPartitionsResponse> PrefetchPartitions(
      PrefetchPartitionsRequest request,
      PrefetchPartitionsStatusCallback status_callback = nullptr);

  /**
   * @brief Removes the partition from the mutable disk cache.
   *
   * @param partition_id The partition ID that should be removed.
   *
   * @note Before calling the API, specify a catalog version. You can set it
   * using the constructor or after the first online request.
   *
   * @return True if partition data is removed successfully; false otherwise.
   */
  bool RemoveFromCache(const std::string& partition_id);

  /**
   * @brief Removes the tile from the mutable disk cache.
   *
   * @param tile The tile key that should be removed.
   *
   * @note Before calling the API, specify a catalog version. You can set it
   * using the constructor or after the first online request.
   *
   * @return True if tile data is removed successfully; false otherwise.
   */
  bool RemoveFromCache(const geo::TileKey& tile);

  /**
   * @brief Removes the partition from the mutable disk cache.
   *
   * @param partition_id The partition ID that should be removed.
   *
   * @note Before calling the API, specify a catalog version. You can set it
   * using the constructor or after the first online request.
   *
   * @return An error if the partition data could not be removed from the cache.
   */
  client::ApiNoResponse DeleteFromCache(const std::string& partition_id);

  /**
   * @brief Removes the tile from the mutable disk cache.
   *
   * @param tile The tile key that should be removed.
   *
   * @note Before calling the API, specify a catalog version. You can set it
   * using the constructor or after the first online request.
   *
   * @return An error if the tile data could not be removed from the cache.
   */
  client::ApiNoResponse DeleteFromCache(const geo::TileKey& tile);

  /**
   * @brief Checks whether the partition is cached.
   *
   * @param partition_id The partition ID.
   *
   * @note Before calling the API, specify a catalog version. You can set it
   * using the constructor or after the first online request.
   *
   * @return True if the partition data is cached; false otherwise.
   */
  bool IsCached(const std::string& partition_id) const;

  /**
   * @brief Checks whether the tile is cached.
   *
   * @param tile The tile key.
   * @param aggregated The aggregated flag, used to specify whether the tile is
   * aggregated or not.
   *
   * @note Before calling the API, specify a catalog version. You can set it
   * using the constructor or after the first online request.
   *
   * @return True if the tile data is cached; false otherwise.
   */
  bool IsCached(const geo::TileKey& tile, bool aggregated = false) const;

  /**
   * @brief Protects tile keys from eviction.
   *
   * Protecting tile keys means that its data and corresponding quadtree
   * keys are added to the protected list and stored in the cache. These keys
   * are removed from the LRU cache, so they could not be evicted. Also, they do
   * not expire. The quadtree stays protected if at least one tile key is
   * protected.
   *
   * @note Before calling the API, specify a catalog version. You can set it
   * using the constructor or after the first online request.
   *
   * @note You can only protect tiles which data handles are present in the
   * cache at the time of the call.
   *
   * @note Please do not call `Protect` while the `Release` call for the same
   * catalog and layer is in progress.
   *
   * @param tiles The list of tile keys to be protected.
   *
   * @return True if some keys were successfully added to the protected list;
   * false otherwise.
   */
  bool Protect(const TileKeys& tiles);

  /**
   * @brief Protect partition from eviction.
   *
   * Protecting partition means that its data and metadata keys are added to the
   * protected list and stored in the cache. These keys are removed from the LRU
   * cache, so they could not be evicted. Also, they do not expire.
   *
   * @note Before calling the API, specify a catalog version. You can set it
   * using the constructor or after the first online request.
   *
   * @note You can only protect partitions which data handles are present in the
   * cache at the time of the call.
   *
   * @note Please do not call `Protect` while the `Release` call for the same
   * catalog and layer is in progress.
   *
   * @param partition_id Partition id to be protected.
   *
   * @return True if partition keys were successfully added to the protected
   * list; false otherwise.
   */
  bool Protect(const std::string& partition_id);

  /**
   * @brief Protect partitions from eviction.
   *
   * Protecting partitions means that its data and metadata keys are added to
   * the protected list and stored in the cache. These keys are removed from the
   * LRU cache, so they could not be evicted. Also, they do not expire.
   *
   * @note Before calling the API, specify a catalog version. You can set it
   * using the constructor or after the first online request.
   *
   * @note You can only protect partitions which data handles are present in the
   * cache at the time of the call.
   *
   * @note Please do not call `Protect` while the `Release` call for the same
   * catalog and layer is in progress.
   *
   * @param partition_ids Partition ids to be protected.
   *
   * @return True if partition keys were successfully added to the protected
   * list; false otherwise.
   */
  bool Protect(const std::vector<std::string>& partition_ids);

  /**
   * @brief Removes a list of tiles from protection.
   *
   * Releasing tile keys removes data and quadtree keys from the protected
   * list. The keys are added to the LRU cache, so they could be evicted.
   * Expiration value is restored, and keys can expire. The quadtree can be
   * removed from the protected list if all tile keys are no longer protected.
   *
   * @note Before calling the API, specify a catalog version. You can set it
   * using the constructor or after the first online request.
   *
   * @note Please make sure that `Protect` will not be called for the same
   * catalog and layer while the `Release` call is in progress.
   *
   * @param tiles The list of tile keys to be removed from protection.
   *
   * @return True if some keys were successfully removed from the protected
   * list; false otherwise.
   */
  bool Release(const TileKeys& tiles);

  /**
   * @brief Removes partition from protection.
   *
   * Releasing partition id removes data handle and metadata keys from the
   * protected list. The keys are added to the LRU cache, so they could be
   * evicted. Expiration value is restored, and related to partition keys can
   * expire.
   *
   * @note Before calling the API, specify a catalog version. You can set it
   * using the constructor or after the first online request.
   *
   * @note Please make sure that `Protect` will not be called for the same
   * catalog and layer while the `Release` call is in progress.
   *
   * @param partition_id Partition id to be removed from protection.
   *
   * @return True if keys related to partition were successfully removed from
   * the protected list; false otherwise.
   */
  bool Release(const std::string& partition_id);

  /**
   * @brief Removes partitions from protection.
   *
   * Releasing partition ids removes data handle and metadata keys from the
   * protected list. The keys are added to the LRU cache, so they could be
   * evicted. Expiration value is restored, and related to partition keys can
   * expire.
   *
   * @note Before calling the API, specify a catalog version. You can set it
   * using the constructor or after the first online request.
   *
   * @note Please make sure that `Protect` will not be called for the same
   * catalog and layer while the `Release` call is in progress.
   *
   * @param partition_ids Partition ids to be removed from protection.
   *
   * @return True if keys related to partitions were successfully removed from
   * the protected list; false otherwise.
   */
  bool Release(const std::vector<std::string>& partition_ids);

 private:
  std::unique_ptr<VersionedLayerClientImpl> impl_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
