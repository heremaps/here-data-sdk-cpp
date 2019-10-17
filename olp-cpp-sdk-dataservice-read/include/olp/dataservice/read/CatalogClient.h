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

#include "DataServiceReadApi.h"
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/geo/tiling/TileKey.h>
#include "olp/dataservice/read/model/Catalog.h"
#include "olp/dataservice/read/model/Data.h"
#include "olp/dataservice/read/model/Partitions.h"
#include "olp/dataservice/read/model/VersionResponse.h"

namespace olp {

namespace client {
class HRN;
}  // namespace client

namespace dataservice {
namespace read {

namespace model {
class Partition;
}  // namespace model

class CatalogRequest;
using CatalogResult = model::Catalog;
using CatalogResponse = client::ApiResponse<CatalogResult, client::ApiError>;
using CatalogResponseCallback = std::function<void(CatalogResponse response)>;

class CatalogVersionRequest;
using CatalogVersionResult = model::VersionResponse;
using CatalogVersionResponse =
    client::ApiResponse<CatalogVersionResult, client::ApiError>;
using CatalogVersionCallback =
    std::function<void(CatalogVersionResponse response)>;

class PartitionsRequest;
using PartitionsResult = model::Partitions;
using PartitionsResponse =
    client::ApiResponse<PartitionsResult, client::ApiError>;
using PartitionsResponseCallback =
    std::function<void(PartitionsResponse response)>;

class DataRequest;
using DataResult = model::Data;
using DataResponse = client::ApiResponse<DataResult, client::ApiError>;
using DataResponseCallback = std::function<void(DataResponse response)>;

class PrefetchTilesRequest;
class DATASERVICE_READ_API PrefetchTileNoError {
 public:
  PrefetchTileNoError() = default;
  PrefetchTileNoError(const PrefetchTileNoError& other) = default;
  ~PrefetchTileNoError() = default;
};

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
  geo::TileKey tile_key_;
};

/**
 * @brief The PrefetchTilesResponse class encapsulates the result of a prefetch
 * operation
 */
using PrefetchTilesResult = std::vector<std::shared_ptr<PrefetchTileResult>>;
using PrefetchTilesResponse =
    client::ApiResponse<PrefetchTilesResult, client::ApiError>;

/**
 * @brief Prefetch completion callback
 */
using PrefetchTilesResponseCallback =
    std::function<void(const PrefetchTilesResponse& response)>;

class CatalogClientImpl;

/**
 * @brief The CatalogClient class. Marshals Requests and their Results.
 */
class DATASERVICE_READ_API CatalogClient final {
 public:
  CatalogClient(client::HRN catalog, client::OlpClientSettings settings);

  ~CatalogClient() = default;

  bool cancelPendingRequests();

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
  client::CancellationToken GetCatalogMetadataVersion(
      const CatalogVersionRequest& request,
      const CatalogVersionCallback& callback);

  /**
   * @brief fetches catalog version asynchronously.
   * @param request contains the complete set of request parameters.
   * @return CancellableFuture of type CatalogVersionResponse, which when
   * complete will contain the catalog configuration or an error.
   * Alternatively, the CancellableFuture can be used to cancel this request.
   */
  client::CancellableFuture<CatalogVersionResponse> GetCatalogMetadataVersion(
      const CatalogVersionRequest& request);

  /**
   * @brief fetches a list partitions for given generic layer asynchronously.
   * @param request contains the complete set of request parameters.
   * @param callback will be invoked once the list of partitions is available,
   * or an error is encountered.
   * @return A token that can be used to cancel this request
   */
  client::CancellationToken GetPartitions(
      const PartitionsRequest& request,
      const PartitionsResponseCallback& callback);

  /**
   * @brief fetches a list of partitions for given generic layer asynchronously.
   * @param request contains the complete set of request parameters.
   * @return CancellableFuture of type PartitionsResponse, which when complete
   * will contain the list of partitions or an error. Alternatively, the
   * CancellableFuture can be used to cancel this request.
   */
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
   */
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
   */
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
   */
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
   */
  client::CancellableFuture<PrefetchTilesResponse> PrefetchTiles(
      const PrefetchTilesRequest& request);

 private:
  std::shared_ptr<CatalogClientImpl> impl_;
};

}  // namespace read

}  // namespace dataservice

}  // namespace olp
