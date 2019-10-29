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
#include <olp/dataservice/read/Types.h>

namespace olp {
namespace dataservice {
namespace read {
class VersionedLayerClientImpl;

/**
 * @brief Client that acquires data from an OLP versioned layer. The versioned
 * layer stores slowly-changing data that must remain logically consistent with
 * other layers in a catalog.
 *
 * @note When you request a particular version of data from a versioned layer,
 * the partition that gets returned may have a lower version number than you
 * requested. Only those layers and partitions that are updated have their
 * version updated to the catalog new version number. The version of a
 * layer or partition represents the catalog version in which the layer or
 * partition was last updated.
 *
 * @note If a catalog version is not specified, the latest version is used. To
 * query the latest version of a catalog, an additional request to OLP is
 * needed.
 *
 * Example with version provided that saves one network request:
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
 * VersionedLayerClient client{"hrn:here:data:::your-catalog-hrn" "your-layer-id", client_settings};
 * auto callback = [](olp::client::ApiResponse<olp::model::Data, olp::client::ApiError> response) {};
 * auto request = DataRequest().WithVersion(100).WithPartitionId("269");
 * auto token = client.GetData(request, callback);
 * @endcode
 *
 * @see
 * https://developer.here.com/olp/documentation/data-api/data_dev_guide/shared_content/topics/olp/concepts/layers.html
 * @see
 * https://developer.here.com/olp/documentation/data-api/data_dev_guide/rest/getting-data-versioned.html
 */
class DATASERVICE_READ_API VersionedLayerClient final {
 public:
  /**
   * @brief VersionedLayerClient constructor
   * @param catalog HRN of the catalog to which this layer belongs.
   * @param layer_id Layer ID of the versioned layer that is used for all
   * requests.
   * @param settings Settings used to control the client instance behavior.
   */
  VersionedLayerClient(client::HRN catalog, std::string layer_id,
                       client::OlpClientSettings settings);

  ~VersionedLayerClient();

  /**
   * @brief Fetches data for a partition ID or data handle asynchronously.
   *
   * If the specified partition ID or data handle cannot be found in the layer,
   * the callback is invoked with an empty DataResponse (a nullptr result and
   * error). If the partition ID or data handle are not set in the request, the
   * callback is invoked with the error ErrorCode::InvalidRequest. If the
   * version is not specified, an additional request to OLP is created to
   * retrieve the latest available partition version.
   *
   * @param data_request Contains the complete set of the request parameters.
   * @note GetLayerId value of the \c DataRequest is ignored, and the parameter
   * from the constructor is used instead.
   * @param callback Is invoked once the DataResult is available or an error is
   * encountered.
   * @return Token that can be used to cancel the GetData request.
   */
  client::CancellationToken GetData(DataRequest data_request,
                                    DataResponseCallback callback);

  /**
   * @brief Fetches data for a partition or data handle asynchronously. If the
   * specified partition or data handle cannot be found in the layer, the
   * callback will be invoked with an empty DataResponse (nullptr for result and
   * error). If neither Partition Id or Data Handle were set in the request, the
   * callback will be invoked with an error with ErrorCode::InvalidRequest.
   * @param data_request contains the complete set of request parameters.
   * \note \c DataRequest's GetLayerId value will be ignored and the parameter
   * from the constructor will be used instead.
   * @return \c CancellableFuture of type \c DataResponse, which when
   * complete will contain the data or an error. Alternatively, the
   * \c CancellableFuture can be used to cancel this request.
   */
  client::CancellableFuture<DataResponse> GetData(DataRequest data_request);

  /**
   * @brief Fetches a list of partitions for the given generic layer
   * asynchronously.
   * @param partitions_request Contains the complete set of the request
   * parameters.
   * @note GetLayerId value of the \c PartitionsRequest is ignored, and the
   * parameter from the constructor is used instead.
   * @param callback Is invoked once the list of partitions is available or an
   * error is encountered.
   * @return Token that can be used to cancel the GetPartitions request.
   */
  client::CancellationToken GetPartitions(PartitionsRequest partitions_request,
                                          PartitionsResponseCallback callback);

  /**
   * @brief Pre-fetches a set of tiles asychronously.
   *
   * This method recursively downloads all tilekeys from the minLevel to
   * maxLevel parameters of the \c PrefetchTilesRequest. It helps to reduce the
   * network load by using the pre-fetched tiles data from cache.
   *
   * @note This does not guarantee that all tiles are available offline as the
   * cache might overflow and data might be evicted at any point.
   *
   * @param request Contains the complete set of the request parameters.
   * @note GetLayerId value of the \c PrefetchTilesRequest is ignored, and the
   * parameter from the constructor is used instead.
   * @param callback Is invoked once the \c PrefetchTilesResult is available or
   * an error is encountered.
   * @return Token that can be used to cancel this request.
   */
  client::CancellationToken PrefetchTiles(
      PrefetchTilesRequest request, PrefetchTilesResponseCallback callback);

  /**
   * @brief Pre-fetches a set of tiles asychronously.
   *
   * This method recursively downloads all tilekeys from the minLevel to
   * maxLevel specified in the \c PrefetchTilesRequest properties. This helps to
   * reduce the network load by using the pre-fetched tiles data from cache.
   *
   * @note This does not guarantee that all tiles are available offline as the
   * cache might overflow and data might be evicted at any point.
   *
   * @param request Contains the complete set of the request parameters.
   * @note GetLayerId value of the \c PrefetchTilesRequest is ignored, and
   * the parameter from the constructor is used instead.
   * @return CancellableFuture of the \c PrefetchTilesResponse type. When
   * completed, the CancellableFuture contains data or an error. Alternatively,
   * the \c CancellableFuture can be used to cancel this request.
   */
  client::CancellableFuture<PrefetchTilesResponse> PrefetchTiles(
      PrefetchTilesRequest request);

 private:
  std::unique_ptr<VersionedLayerClientImpl> impl_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
