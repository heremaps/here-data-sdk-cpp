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

#include <olp/core/client/ApiLookupClient.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/PendingRequests.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/PrefetchPartitionsRequest.h>
#include <olp/dataservice/read/PrefetchTileResult.h>
#include <olp/dataservice/read/PrefetchTilesRequest.h>
#include <olp/dataservice/read/TileRequest.h>
#include <olp/dataservice/read/Types.h>
#include <boost/optional.hpp>
#include "TaskSink.h"
#include "repositories/NamedMutex.h"

namespace olp {
namespace thread {
class TaskScheduler;
}
namespace dataservice {
namespace read {
namespace repository {
class PartitionsRepository;
class PrefetchTilesRepository;
}  // namespace repository

class VersionedLayerClientImpl {
 public:
  VersionedLayerClientImpl(client::HRN catalog, std::string layer_id,
                           boost::optional<int64_t> catalog_version,
                           client::OlpClientSettings settings);

  virtual ~VersionedLayerClientImpl() = default;

  virtual bool CancelPendingRequests();

  virtual client::CancellationToken GetData(DataRequest request,
                                            DataResponseCallback callback);

  virtual client::CancellableFuture<DataResponse> GetData(
      DataRequest data_request);

  virtual client::CancellationToken GetData(TileRequest request,
                                            DataResponseCallback callback);

  virtual client::CancellableFuture<DataResponse> GetData(TileRequest request);

  virtual client::CancellationToken GetPartitions(
      PartitionsRequest request, PartitionsResponseCallback callback);

  virtual client::CancellationToken StreamLayerPartitions(
      PartitionsRequest request,
      PartitionsStreamCallback partition_stream_callback,
      CallbackNoResult callback);

  virtual client::CancellableFuture<PartitionsResponse> GetPartitions(
      PartitionsRequest partitions_request);

  virtual client::CancellationToken QuadTreeIndex(
      TileRequest tile_request, PartitionsResponseCallback callback);

  virtual client::CancellationToken PrefetchTiles(
      PrefetchTilesRequest request, PrefetchTilesResponseCallback callback,
      PrefetchStatusCallback status_callback);

  virtual client::CancellableFuture<PrefetchTilesResponse> PrefetchTiles(
      PrefetchTilesRequest request, PrefetchStatusCallback status_callback);

  virtual client::CancellationToken PrefetchPartitions(
      PrefetchPartitionsRequest request,
      PrefetchPartitionsResponseCallback callback,
      PrefetchPartitionsStatusCallback status_callback);

  virtual client::CancellableFuture<PrefetchPartitionsResponse>
  PrefetchPartitions(PrefetchPartitionsRequest request,
                     PrefetchPartitionsStatusCallback status_callback);

  virtual bool RemoveFromCache(const std::string& partition_id);

  virtual bool RemoveFromCache(const geo::TileKey& tile);

  virtual bool IsCached(const std::string& partition_id);

  virtual bool IsCached(const geo::TileKey& tile, bool aggregated = false);

  virtual client::CancellationToken GetAggregatedData(
      TileRequest request, AggregatedDataResponseCallback callback);

  virtual client::CancellableFuture<AggregatedDataResponse> GetAggregatedData(
      TileRequest request);

  virtual bool Protect(const TileKeys& tiles);

  virtual bool Release(const TileKeys& tiles);

  virtual bool Protect(const std::vector<std::string>& partition_ids);

  virtual bool Release(const std::vector<std::string>& partition_ids);

 private:
  CatalogVersionResponse GetVersion(boost::optional<std::string> billing_tag,
                                    const FetchOptions& fetch_options,
                                    const client::CancellationContext& context);

  client::HRN catalog_;
  std::string layer_id_;
  client::OlpClientSettings settings_;
  std::atomic<int64_t> catalog_version_;
  client::ApiLookupClient lookup_client_;
  repository::NamedMutexStorage mutex_storage_;
  TaskSink task_sink_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
