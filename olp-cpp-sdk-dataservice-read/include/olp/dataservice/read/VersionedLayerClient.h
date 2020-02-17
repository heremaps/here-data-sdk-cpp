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

#include <boost/optional.hpp>
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
#include <olp/dataservice/read/Types.h>

namespace olp {
namespace dataservice {
namespace read {
class VersionedLayerClientImpl;

// clang-format off
/**
 * @brief Gets data from a versioned layer of the Open Location Platform (OLP).
 *
 * The versioned layer stores slowly-changing data that must remain logically
 * consistent with other layers in a catalog. You can request any data version
 * from the versioned
 * layer.
 * When you request a particular version of data from the versioned layer,
 * the partition you receive in the response may have a lower version number
 * than you requested. The version of a layer or partition represents the
 * catalog version in which the layer or partition was last updated.
 *
 * @note If the catalog version is not specified, the latest version is used.
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
 * VersionedLayerClient client{"hrn:here:data:::your-catalog-hrn", "your-layer-id", client_settings};
 * auto callback = [](olp::client::ApiResponse<olp::model::Data, olp::client::ApiError> response) {};
 * auto request = DataRequest().WithVersion(100).WithPartitionId("269");
 * auto token = client.GetData(request, callback);
 * @endcode
 *
 * @see
 * The [versioned
 * layer](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/layers.html#versioned-layers)
 * section in the Data User Guide.
 */
// clang-format on
class DATASERVICE_READ_API VersionedLayerClient final {
 public:
  /**
   * @brief Creates the `VersionedLayerClient` instance.
   *
   * @param catalog The HERE Resource Name (HRN) of the catalog that contains
   * the versioned layer from which you want to get data.
   * @param layer_id The layer ID of the versioned layer from which you want to
   * get data.
   * @param settings The `OlpClientSettings` instance.
   *
   * @deprecated Will be removed by 06.2020.
   */
  OLP_SDK_DEPRECATED(
      "Use the ctor with the explicitly specified version. This ctor is "
      "deprecated and will be removed "
      "by 06.2020")
  VersionedLayerClient(client::HRN catalog, std::string layer_id,
                       client::OlpClientSettings settings);
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

  /// Movable, non-copyable
  VersionedLayerClient(const VersionedLayerClient& other) = delete;
  VersionedLayerClient(VersionedLayerClient&& other) noexcept;
  VersionedLayerClient& operator=(const VersionedLayerClient& other) = delete;
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
   * request to OLP is created to retrieve the latest available partition
   * version.
   *
   * @param data_request The `DataRequest` instance that contains a complete set
   * of request parameters.
   * @note The `GetLayerId` value of the \c DataRequest object is ignored, and
   * the parameter from the constructor is used instead.
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
   * @note The `GetLayerId` value of the \c DataRequest object is ignored, and
   * the parameter from the constructor is used instead.
   *
   * @return `CancellableFuture` that contains the `DataResponse` instance
   * or an error. You can also use `CancellableFuture` to cancel this request.
   */
  client::CancellableFuture<DataResponse> GetData(DataRequest data_request);

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
   * @note The `GetLayerId` value of the \c PartitionsRequest object is ignored,
   * and the parameter from the constructor is used instead.
   * @param callback The `PartitionsResponseCallback` object that is invoked if
   * the list of partitions is available or an error is encountered.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken GetPartitions(PartitionsRequest partitions_request,
                                          PartitionsResponseCallback callback);

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
   * @note The `GetLayerId` value of the \c PartitionsRequest object is ignored,
   * and the parameter from the constructor is used instead.
   *
   * @return `CancellableFuture` that contains the `PartitionsResponse` instance
   * with data or an error. You can also use `CancellableFuture` to cancel this
   * request.
   */
  client::CancellableFuture<PartitionsResponse> GetPartitions(
      PartitionsRequest partitions_request);

  /**
   * @brief Prefetches a set of tiles asynchronously.
   *
   * This method recursively downloads all tile keys from the `minLevel`
   * parameter to the `maxLevel` parameter of the \c PrefetchTilesRequest
   * object. It helps to reduce the network load by using the prefetched tiles
   * data from the cache.
   *
   * @note This method does not guarantee that all tiles are available offline
   * as the cache might overflow, and data might be evicted at any point.
   *
   * @param request The `PrefetchTilesRequest` instance that contains
   * a complete set of request parameters.
   * @note The `GetLayerId` value of the \c PrefetchTilesRequest object is
   * ignored, and the parameter from the constructor is used instead.
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
   * This method recursively downloads all tile keys from the `minLevel`
   * parameter to the `maxLevel` parameter of the \c PrefetchTilesRequest
   * object. It helps to reduce the network load by using the prefetched tiles
   * data from the cache.
   *
   * @note This method does not guarantee that all tiles are available offline
   * as the cache might overflow, and data might be evicted at any point.
   *
   * @param request The `PrefetchTilesRequest` instance that contains
   * a complete set of request parameters.
   * @note The `GetLayerId` value of the \c PrefetchTilesRequest object is
   * ignored, and the parameter from the constructor is used instead.
   *
   * @return `CancellableFuture` that contains the `PrefetchTilesResponse`
   * instance with data or an error. You can also use `CancellableFuture` to
   * cancel this request.
   */
  client::CancellableFuture<PrefetchTilesResponse> PrefetchTiles(
      PrefetchTilesRequest request);

 private:
  std::unique_ptr<VersionedLayerClientImpl> impl_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
