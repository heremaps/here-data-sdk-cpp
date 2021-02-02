/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiNoResult.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/porting/deprecated.h>

#include <olp/dataservice/write/DataServiceWriteApi.h>
#include <olp/dataservice/write/generated/model/Publication.h>
#include <olp/dataservice/write/generated/model/ResponseOk.h>
#include <olp/dataservice/write/generated/model/ResponseOkSingle.h>
#include <olp/dataservice/write/model/CheckDataExistsRequest.h>
#include <olp/dataservice/write/model/PublishPartitionDataRequest.h>
#include <olp/dataservice/write/model/StartBatchRequest.h>
#include <olp/dataservice/write/model/VersionResponse.h>

namespace olp {
namespace dataservice {
namespace write {

using StartBatchResult = model::Publication;
using StartBatchResponse =
    client::ApiResponse<StartBatchResult, client::ApiError>;
using StartBatchCallback = std::function<void(StartBatchResponse)>;

using GetBaseVersionResult = model::VersionResponse;
using GetBaseVersionResponse =
    client::ApiResponse<GetBaseVersionResult, client::ApiError>;
using GetBaseVersionCallback = std::function<void(GetBaseVersionResponse)>;

using GetBatchResult = model::Publication;
using GetBatchResponse = client::ApiResponse<GetBatchResult, client::ApiError>;
using GetBatchCallback = std::function<void(GetBatchResponse)>;

using CompleteBatchResult = client::ApiNoResult;
using CompleteBatchResponse =
    client::ApiResponse<CompleteBatchResult, client::ApiError>;
using CompleteBatchCallback =
    std::function<void(CompleteBatchResponse response)>;

using CancelBatchResult = client::ApiNoResult;
using CancelBatchResponse =
    client::ApiResponse<CancelBatchResult, client::ApiError>;
using CancelBatchCallback = std::function<void(CancelBatchResponse response)>;

using PublishPartitionDataResponse =
    client::ApiResponse<model::ResponseOkSingle, client::ApiError>;
using PublishPartitionDataCallback =
    std::function<void(PublishPartitionDataResponse response)>;

using CheckDataExistsStatusCode = int;
using CheckDataExistsResponse =
    client::ApiResponse<CheckDataExistsStatusCode, client::ApiError>;
using CheckDataExistsCallback =
    std::function<void(CheckDataExistsResponse response)>;

class VersionedLayerClientImpl;

/**
 * @brief Client class to ingest data into versioned layers.
 */
class DATASERVICE_WRITE_API VersionedLayerClient {
 public:
  /**
   * @brief VersionedLayerClient constructor
   * @param catalog the catalog this versioned layer client uses
   * @param settings Client settings used to control behaviour of the client
   * instance
   */
  VersionedLayerClient(client::HRN catalog, client::OlpClientSettings settings);

  /**
   * @brief Start a batch operation.
   * @param request details of the batch operation to start
   * @return olp::client::CancellableFuture<StartBatchResponse>
   */
  olp::client::CancellableFuture<StartBatchResponse> StartBatch(
      model::StartBatchRequest request);

  /**
   * @brief Start a batch operation.
   * @param request details of the batch operation to start
   * @param callback of type std::function<void(StartBatchResponse response)>
   * @return olp::client::CancellationToken
   */
  olp::client::CancellationToken StartBatch(model::StartBatchRequest request,
                                            StartBatchCallback callback);

  /**
   * @brief Get the latest version number of the catalog
   * @return future holding the response object
   */
  olp::client::CancellableFuture<GetBaseVersionResponse> GetBaseVersion();

  /**
   * @brief Get the latest version number of the catalog
   * @param callback called when operation completes
   * @return cancellationToken
   */
  olp::client::CancellationToken GetBaseVersion(
      GetBaseVersionCallback callback);

  /**
   * @brief Get the details of the given batch publication
   * @param pub the publication to get the current details of
   * @return future returning a batch response.
   */
  olp::client::CancellableFuture<GetBatchResponse> GetBatch(
      const model::Publication& pub);

  /**
   * @brief Get the details of the given batch publication
   * @param pub the publication to get the current details of
   * @param callback called when the operation completes
   * @return cancellation token
   */
  olp::client::CancellationToken GetBatch(const model::Publication& pub,
                                          GetBatchCallback callback);

  /**
   * @brief Complete the given batch operation and commit to the HERE platform.
   * @param pub the publication to complete
   * @return future containing the batch response
   */
  olp::client::CancellableFuture<CompleteBatchResponse> CompleteBatch(
      const model::Publication& pub);

  /**
   * @brief Complete the given batch operation and commit to the HERE platform.
   * @param pub the publication to complete
   * @param callback called when the operation completes.
   * @return cancellation token
   */
  olp::client::CancellationToken CompleteBatch(const model::Publication& pub,
                                               CompleteBatchCallback callback);

  /**
   * @brief Cancel the given batch operation
   * @param pub the publication to cancel
   * @return future containing the batch response
   */
  olp::client::CancellableFuture<CancelBatchResponse> CancelBatch(
      const model::Publication& pub);

  /**
   * @brief Cancel the given batch operation
   * @param pub the publication to cancel
   * @param callback called when the operation cancel
   * @return cancellation token
   */
  olp::client::CancellationToken CancelBatch(const model::Publication& pub,
                                             CancelBatchCallback callback);

  /**
   * @brief Cancels all the ongoing operations that this client started.
   *
   * Returns instantly and does not wait for the callbacks.
   * Use this operation to cancel all the pending requests without
   * destroying the actual client instance.
   */
  void CancelPendingRequests();

  /**
   * @brief Call to publish data into a versioned layer.
   * @note Content-type for this request will be set implicitly based on the
   * layer metadata for the target layer on the HERE platform.
   * @param request PublishPartitionDataRequest object representing the
   * parameters for this publishData call.
   *
   * @return A CancellableFuture containing the PublishPartitionDataResponse.
   */
  olp::client::CancellableFuture<PublishPartitionDataResponse> PublishToBatch(
      const model::Publication& pub,
      model::PublishPartitionDataRequest request);

  /**
   * @brief Call to publish data into a versioned layer.
   * @note Content-type for this request will be set implicitly based on the
   * layer metadata for the target layer on the HERE platform.
   * @param request PublishPartitionDataRequest object representing the
   * parameters for this publishData call.
   * @param callback PublishPartitionDataCallback which will be called with the
   * PublishPartitionDataResponse when the operation completes.
   *
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken PublishToBatch(
      const model::Publication& pub, model::PublishPartitionDataRequest request,
      PublishPartitionDataCallback callback);

  /**
   * @brief Check if a datahandle exits.
   * @param request details of the check data exists operation to start
   * @return olp::client::CancellableFuture<CheckDataExistsResponse>
   */
  olp::client::CancellableFuture<CheckDataExistsResponse> CheckDataExists(
      model::CheckDataExistsRequest request);

  /**
   * @brief Check if a datahandle exits.
   * @param request details of the check data exists operation to start
   * @param callback of type std::function<void(CheckDataResponse response)>
   * @return olp::client::CancellationToken
   */
  olp::client::CancellationToken CheckDataExists(
      model::CheckDataExistsRequest request, CheckDataExistsCallback callback);

 private:
  std::shared_ptr<VersionedLayerClientImpl> impl_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
