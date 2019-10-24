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

#include <PendingRequests.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/PrefetchTileResult.h>
#include <olp/dataservice/read/PrefetchTilesRequest.h>
#include <olp/dataservice/read/model/Data.h>
#include <olp/dataservice/read/model/Partitions.h>

namespace olp {
namespace thread {
class TaskScheduler;
}
namespace dataservice {
namespace read {
namespace repository {
class PartitionsRepository;
}

class PrefetchTilesProvider;

class VersionedLayerClientImpl {
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

  VersionedLayerClientImpl(client::HRN catalog, std::string layer_id,
                           client::OlpClientSettings settings);

  virtual ~VersionedLayerClientImpl();

  virtual client::CancellationToken GetData(DataRequest data_request,
                                            Callback callback) const;

  virtual client::CancellationToken GetPartitions(
      PartitionsRequest partitions_request, PartitionsCallback callback) const;

  virtual client::CancellationToken PrefetchTiles(
      PrefetchTilesRequest request, PrefetchTilesResponseCallback callback);

 protected:
  client::HRN catalog_;
  std::string layer_id_;
  std::shared_ptr<client::OlpClientSettings> settings_;
  std::shared_ptr<thread::TaskScheduler> task_scheduler_;
  std::shared_ptr<PendingRequests> pending_requests_;
  std::shared_ptr<repository::PartitionsRepository> partition_repo_;
  std::shared_ptr<PrefetchTilesProvider> prefetch_provider_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
