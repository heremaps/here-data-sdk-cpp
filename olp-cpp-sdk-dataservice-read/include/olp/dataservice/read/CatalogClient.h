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

#include <functional>
#include <memory>

#include <olp/core/cache/CacheSettings.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/porting/deprecated.h>
#include "DataServiceReadApi.h"
#include "olp/dataservice/read/PrefetchTileResult.h"
#include "olp/dataservice/read/Types.h"

namespace olp {

namespace client {
class HRN;
}  // namespace client

namespace dataservice {
namespace read {

class CatalogClientImpl;

class CatalogRequest;
class CatalogVersionRequest;
class DataRequest;
class PartitionsRequest;
class PrefetchTilesRequest;

/**
 * @brief The CatalogClient class. Marshals Requests and their Results.
 */
class DATASERVICE_READ_API CatalogClient final {
 public:
  CatalogClient(client::HRN catalog, client::OlpClientSettings settings);

  // Movable, non-copyable
  CatalogClient(const CatalogClient& other) = delete;
  CatalogClient(CatalogClient&& other) noexcept;
  CatalogClient& operator=(const CatalogClient& other) = delete;
  CatalogClient& operator=(CatalogClient&& other) noexcept;

  ~CatalogClient() = default;

  /**
   * @brief Cancel currently active requests.
   * @return True on success
   */
  bool CancelPendingRequests();

  /**
   * @brief fetches catalog configuration asynchronously.
   * @param request contains the complete set of request parameters.
   * @param callback will be invoked once the catalog configuration is
   * available, or an error is encountered.
   * @return A token that can be used to cancel this request
   */
  client::CancellationToken GetCatalog(const CatalogRequest& request,
                                       const CatalogResponseCallback& callback);

  /**
   * @brief fetches catalog configuration asynchronously.
   * @param request contains the complete set of request parameters.
   * @return CancellableFuture of type CatalogResponse, which when complete will
   * contain the catalog configuration or an error. Alternatively, the
   * CancellableFuture can be used to cancel this request.
   */
  client::CancellableFuture<CatalogResponse> GetCatalog(
      const CatalogRequest& request);

  /**
   * @brief fetches catalog version asynchronously.
   * @param request contains the complete set of request parameters.
   * @param callback will be invoked once the catalog version is
   * available, or an error is encountered.
   * @return A token that can be used to cancel this request
   */
  client::CancellationToken GetLatestVersion(CatalogVersionRequest request,
                                             CatalogVersionCallback callback);

  /**
   * @brief fetches catalog version asynchronously.
   * @param request contains the complete set of request parameters.
   * @return CancellableFuture of type CatalogVersionResponse, which when
   * complete will contain the catalog configuration or an error.
   * Alternatively, the CancellableFuture can be used to cancel this request.
   */
  client::CancellableFuture<CatalogVersionResponse> GetLatestVersion(
      CatalogVersionRequest request);

  /**
   * @brief fetches catalog version asynchronously.
   * @param request contains the complete set of request parameters.
   * @param callback will be invoked once the catalog version is
   * available, or an error is encountered.
   * @return A token that can be used to cancel this request
   * @deprecated Will be removed in 1.1, please use \c GetLatestVersion instead.
   */
  OLP_SDK_DEPRECATED(
      "Will be removed in 1.1, please use GetLatestVersion() instead")
  client::CancellationToken GetCatalogMetadataVersion(
      const CatalogVersionRequest& request,
      const CatalogVersionCallback& callback);

  /**
   * @brief fetches catalog version asynchronously.
   * @param request contains the complete set of request parameters.
   * @return CancellableFuture of type CatalogVersionResponse, which when
   * complete will contain the catalog configuration or an error.
   * Alternatively, the CancellableFuture can be used to cancel this request.
   * @deprecated Will be removed in 1.1, please use \c GetLatestVersion instead.
   */
  OLP_SDK_DEPRECATED(
      "Will be removed in 1.1, please use GetLatestVersion() instead")
  client::CancellableFuture<CatalogVersionResponse> GetCatalogMetadataVersion(
      const CatalogVersionRequest& request);

  /**
   * @brief fetches a list partitions for given generic layer asynchronously.
   * @param request contains the complete set of request parameters.
   * @param callback will be invoked once the list of partitions is available,
   * or an error is encountered.
   * @return A token that can be used to cancel this request
   * @deprecated Will be removed in 1.0
   */
  OLP_SDK_DEPRECATED("Will be removed in 1.0")
  client::CancellationToken GetPartitions(
      const PartitionsRequest& request,
      const PartitionsResponseCallback& callback);

  /**
   * @brief fetches a list of partitions for given generic layer asynchronously.
   * @param request contains the complete set of request parameters.
   * @return CancellableFuture of type PartitionsResponse, which when complete
   * will contain the list of partitions or an error. Alternatively, the
   * CancellableFuture can be used to cancel this request.
   * @deprecated Will be removed in 1.0
   */
  OLP_SDK_DEPRECATED("Will be removed in 1.0")
  client::CancellableFuture<PartitionsResponse> GetPartitions(
      const PartitionsRequest& request);

  /**
   * @brief fetches data for a partition or data handle asynchronously. If the
   * specified partition or data handle cannot be found in the layer, the
   * callback will be invoked with an empty DataResponse (nullptr for result and
   * error). If neither Partition Id or Data Handle were set in the request, the
   * callback will be invoked with an error with ErrorCode::InvalidRequest.
   * @param request contains the complete set of request parameters.
   * @param callback will be invoked once the DataResult is available, or an
   * error is encountered.
   * @return A token that can be used to cancel this request
   * @deprecated Will be removed in 1.0
   */
  OLP_SDK_DEPRECATED("Will be removed in 1.0")
  client::CancellationToken GetData(const DataRequest& request,
                                    const DataResponseCallback& callback);

  /**
   * @brief fetches data for a partition or data handle asynchronously. If the
   * specified partition or data handle cannot be found in the layer, an empty
   * DataResponse (nullptr for result and error) will be returned. If neither
   * Partition Id or Data Handle were set in the request, an error with code
   * ErrorCode::InvalidRequest will be returned.
   * @param request contains the complete set of request parameters.
   * @return CancellableFuture of type DataResponse, which when complete will
   * contain the data or an error. Alternatively, the CancellableFuture can be
   * used to cancel this request.
   * @deprecated Will be removed in 1.0
   */
  OLP_SDK_DEPRECATED("Will be removed in 1.0")
  client::CancellableFuture<DataResponse> GetData(const DataRequest& request);

  /**
   * @brief pre-fetches a set of tiles asynchronously
   *
   * Recursively downloads all tilekeys up to maxLevel.
   *
   * Note - this does not guarantee that all tiles are available offline, as the
   * cache might overflow and data might be evicted at any point.
   *
   * @param request contains the complete set of request parameters.
   * @param callback the callback to involve when the operation finished
   * @param statusCallback the callback to involve to indicate status of the
   * operation
   * @return A token that can be used to cancel this prefetch request
   * @deprecated Will be removed in 1.0
   */
  OLP_SDK_DEPRECATED("Will be removed in 1.0")
  client::CancellationToken PrefetchTiles(
      const PrefetchTilesRequest& request,
      const PrefetchTilesResponseCallback& callback);

  /**
   * @brief pre-fetches a set of tiles asynchronously
   *
   * Recursively downloads all tilekeys up to maxLevel.
   *
   * Note - this does not guarantee that all tiles are available offline, as the
   * cache might overflow and data might be evicted at any point.
   *
   * @param request contains the complete set of request parameters.
   * @return CancellableFuture of type PrefetchTilesResponse, which when
   * complete will contain the data or an error. Alternatively, the
   * CancellableFuture can be used to cancel this request.
   * @deprecated Will be removed in 1.0
   */
  OLP_SDK_DEPRECATED("Will be removed in 1.0")
  client::CancellableFuture<PrefetchTilesResponse> PrefetchTiles(
      const PrefetchTilesRequest& request);

 private:
  std::shared_ptr<CatalogClientImpl> impl_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
