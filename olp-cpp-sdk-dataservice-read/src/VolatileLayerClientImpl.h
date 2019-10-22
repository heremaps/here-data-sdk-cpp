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

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/model/Data.h>
#include <olp/dataservice/read/model/Partitions.h>

#include "PendingRequests.h"

namespace olp {
namespace dataservice {
namespace read {

namespace repository {
class CatalogRepository;
class PartitionsRepository;
}  // namespace repository

class VolatileLayerClientImpl {
 public:
  /// DataResult alias
  using DataResult = model::Data;
  /// CallbackResponse alias
  using DataResponse = client::ApiResponse<DataResult, client::ApiError>;
  /// PartitionResult alias
  using PartitionsResult = model::Partitions;
  /// CallbackResponse
  using PartitionsResponse =
      client::ApiResponse<PartitionsResult, client::ApiError>;
  /// Callback alias
  template <class Response>
  using Callback = std::function<void(Response response)>;

  VolatileLayerClientImpl(client::HRN catalog, std::string layer_id,
                          client::OlpClientSettings settings);

  ~VolatileLayerClientImpl();

  client::CancellationToken GetPartitions(
      PartitionsRequest request, Callback<PartitionsResponse> callback);

  client::CancellationToken GetData(DataRequest request,
                                    Callback<DataResponse> callback);

 private:
  client::HRN catalog_;
  std::string layer_id_;
  std::shared_ptr<client::OlpClientSettings> settings_;
  std::shared_ptr<thread::TaskScheduler> task_scheduler_;
  std::shared_ptr<PendingRequests> pending_requests_;
  std::shared_ptr<repository::PartitionsRepository> partition_repo_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
