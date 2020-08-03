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

#include <olp/core/client/ApiLookupClient.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/PrefetchTilesRequest.h>
#include <olp/dataservice/read/Types.h>

namespace olp {

namespace client {
class PendingRequests;
}
namespace dataservice {
namespace read {

namespace repository {
class CatalogRepository;
class PartitionsRepository;
}  // namespace repository

class VolatileLayerClientImpl {
 public:
  VolatileLayerClientImpl(client::HRN catalog, std::string layer_id,
                          client::OlpClientSettings settings);

  virtual ~VolatileLayerClientImpl();

  virtual bool CancelPendingRequests();

  virtual client::CancellationToken GetPartitions(
      PartitionsRequest request, PartitionsResponseCallback callback);

  virtual client::CancellableFuture<PartitionsResponse> GetPartitions(
      PartitionsRequest request);

  virtual client::CancellationToken GetData(DataRequest request,
                                            DataResponseCallback callback);

  virtual client::CancellableFuture<DataResponse> GetData(DataRequest request);

  virtual bool RemoveFromCache(const std::string& partition_id);

  virtual bool RemoveFromCache(const geo::TileKey& tile);

  virtual client::CancellationToken PrefetchTiles(
      PrefetchTilesRequest request, PrefetchTilesResponseCallback callback);

  virtual client::CancellableFuture<PrefetchTilesResponse> PrefetchTiles(
      PrefetchTilesRequest request);

 private:
  client::HRN catalog_;
  std::string layer_id_;
  client::OlpClientSettings settings_;
  std::shared_ptr<client::PendingRequests> pending_requests_;
  client::ApiLookupClient lookup_client_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
