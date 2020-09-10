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

#include <olp/dataservice/write/VersionedLayerClient.h>

#include <generated/model/PublishPartition.h>
#include <generated/model/PublishPartitions.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/PendingRequests.h>
#include "CancellationTokenList.h"
#include "CatalogSettings.h"
#include "generated/model/Catalog.h"

#include <condition_variable>
#include <memory>
#include <mutex>

namespace olp {
namespace dataservice {
namespace write {

using InitApiClientsCallback =
    std::function<void(boost::optional<client::ApiError>)>;
using InitCatalogModelCallback =
    std::function<void(boost::optional<client::ApiError>)>;

using UploadPartitionResult = client::ApiNoResult;
using UploadPartitionResponse =
    client::ApiResponse<UploadPartitionResult, client::ApiError>;
using UploadPartitionCallback =
    std::function<void(UploadPartitionResponse response)>;

using UploadBlobResult = client::ApiNoResult;
using UploadBlobResponse =
    client::ApiResponse<UploadBlobResult, client::ApiError>;
using UploadBlobCallback = std::function<void(UploadBlobResponse response)>;

class VersionedLayerClientImpl
    : public std::enable_shared_from_this<VersionedLayerClientImpl> {
 public:
  VersionedLayerClientImpl(client::HRN catalog,
                           client::OlpClientSettings settings);

  virtual ~VersionedLayerClientImpl();

  client::CancellableFuture<StartBatchResponse> StartBatch(
      const model::StartBatchRequest& request);

  client::CancellationToken StartBatch(const model::StartBatchRequest& request,
                                       StartBatchCallback callback);

  client::CancellableFuture<GetBaseVersionResponse> GetBaseVersion();

  client::CancellationToken GetBaseVersion(GetBaseVersionCallback callback);

  client::CancellableFuture<GetBatchResponse> GetBatch(
      const model::Publication& pub);

  client::CancellationToken GetBatch(const model::Publication& pub,
                                     GetBatchCallback callback);

  client::CancellableFuture<CompleteBatchResponse> CompleteBatch(
      const model::Publication& pub);

  client::CancellationToken CompleteBatch(const model::Publication& publication,
                                          CompleteBatchCallback callback);

  client::CancellableFuture<CancelBatchResponse> CancelBatch(
      const model::Publication& pub);

  client::CancellationToken CancelBatch(const model::Publication& pub,
                                        CancelBatchCallback callback);

  void CancelPendingRequests();

  client::CancellableFuture<PublishPartitionDataResponse> PublishToBatch(
      const model::Publication& pub,
      const model::PublishPartitionDataRequest& request);

  client::CancellationToken PublishToBatch(
      const model::Publication& pub,
      const model::PublishPartitionDataRequest& request,
      PublishPartitionDataCallback callback);

  client::CancellableFuture<CheckDataExistsResponse> CheckDataExists(
      const model::CheckDataExistsRequest& request);

  client::CancellationToken CheckDataExists(
      const model::CheckDataExistsRequest& request,
      CheckDataExistsCallback callback);

 private:
  using BillingTag = boost::optional<std::string>;

  client::CancellationToken InitApiClients(
      std::shared_ptr<client::CancellationContext> cancel_context,
      InitApiClientsCallback callback);

  UploadBlobResponse UploadBlob(const model::PublishPartition& partition,
                                const std::string& data_handle,
                                const std::string& content_type,
                                const std::string& content_encoding,
                                const std::string& layer_id,
                                BillingTag billing_tag,
                                client::CancellationContext context);

  UploadPartitionResponse UploadPartition(
      const std::string& publication_id,
      const model::PublishPartition& partition, const std::string& layer_id,
      client::CancellationContext context);

  client::HRN catalog_;
  client::OlpClientSettings settings_;

  CatalogSettings catalog_settings_;

  std::shared_ptr<client::OlpClient> apiclient_blob_;
  std::shared_ptr<client::OlpClient> apiclient_config_;
  std::shared_ptr<client::OlpClient> apiclient_metadata_;
  std::shared_ptr<client::OlpClient> apiclient_publish_;
  std::shared_ptr<client::OlpClient> apiclient_query_;

  CancellationTokenList tokenList_;

  std::shared_ptr<client::PendingRequests> pending_requests_;

  std::mutex mutex_;
  std::condition_variable cond_var_;
  bool init_in_progress_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
