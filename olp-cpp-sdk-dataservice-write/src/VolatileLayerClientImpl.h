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

#include <map>
#include <mutex>

#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/PendingRequests.h>
#include "ApiClientLookup.h"

#include <olp/dataservice/write/VolatileLayerClient.h>
#include "CancellationTokenList.h"
#include "CatalogSettings.h"
#include "generated/model/Catalog.h"

namespace olp {
namespace dataservice {
namespace write {

namespace model {
class PublishDataRequest;
class Partition;
}  // namespace model

using InitApiClientsCallback =
    std::function<void(boost::optional<client::ApiError>)>;
using DataHandleMap = std::map<std::string, std::string>;
using DataHandleMapResponse =
    client::ApiResponse<DataHandleMap, client::ApiError>;
using DataHandleMapCallback = std::function<void(DataHandleMapResponse)>;

class VolatileLayerClientImpl
    : public std::enable_shared_from_this<VolatileLayerClientImpl> {
 public:
  VolatileLayerClientImpl(client::HRN catalog,
                          client::OlpClientSettings settings);

  virtual ~VolatileLayerClientImpl();

  olp::client::CancellableFuture<GetBaseVersionResponse> GetBaseVersion();

  olp::client::CancellationToken GetBaseVersion(
      GetBaseVersionCallback callback);

  void CancelAll();

  void CancelPendingRequests();

  olp::client::CancellableFuture<PublishPartitionDataResponse>
  PublishPartitionData(const model::PublishPartitionDataRequest& request);

  olp::client::CancellationToken PublishPartitionData(
      const model::PublishPartitionDataRequest& request,
      PublishPartitionDataCallback callback);

  client::CancellableFuture<StartBatchResponse> StartBatch(
      const model::StartBatchRequest& request);

  olp::client::CancellationToken StartBatch(
      const model::StartBatchRequest& request, StartBatchCallback callback);

  olp::client::CancellableFuture<GetBatchResponse> GetBatch(
      const model::Publication& pub);

  olp::client::CancellationToken GetBatch(const model::Publication& pub,
                                          GetBatchCallback callback);

  olp::client::CancellationToken PublishToBatch(
      const model::Publication& pub,
      const std::vector<model::PublishPartitionDataRequest>& partitions,
      PublishToBatchCallback callback);

  olp::client::CancellableFuture<PublishToBatchResponse> PublishToBatch(
      const model::Publication& pub,
      const std::vector<model::PublishPartitionDataRequest>& partitions);

  olp::client::CancellableFuture<CompleteBatchResponse> CompleteBatch(
      const model::Publication& pub);

  olp::client::CancellationToken CompleteBatch(const model::Publication& pub,
                                               CompleteBatchCallback callback);

 private:
  client::CancellationToken InitApiClients(
      std::shared_ptr<client::CancellationContext> cancel_context,
      InitApiClientsCallback callback);

  DataHandleMapResponse GetDataHandleMap(
      const std::string& layerId, const std::vector<std::string>& partitionIds,
      boost::optional<int64_t> version,
      boost::optional<std::vector<std::string>> additionalFields,
      boost::optional<std::string> billingTag,
      const client::CancellationContext context);

 private:
  client::HRN catalog_;

  client::OlpClientSettings settings_;

  CatalogSettings catalog_settings_;

  std::shared_ptr<client::OlpClient> apiclient_config_;
  std::shared_ptr<client::OlpClient> apiclient_blob_;
  std::shared_ptr<client::OlpClient> apiclient_publish_;
  std::shared_ptr<client::OlpClient> apiclient_metadata_;
  std::shared_ptr<client::OlpClient> apiclient_query_;

  CancellationTokenList tokenList_;

  std::shared_ptr<client::PendingRequests> pending_requests_;
  std::shared_ptr<thread::TaskScheduler> task_scheduler_;

  std::mutex mutex_;
  std::condition_variable cond_var_;

  bool init_in_progress_{false};
};
}  // namespace write
}  // namespace dataservice
}  // namespace olp
