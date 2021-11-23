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

/// Publishes data to a versioned layers.
class DATASERVICE_WRITE_API VersionedLayerClient {
 public:
  /**
   * @brief Creates the `VersionedLayerClient` instance.
   *
   * @param catalog The HRN of the catalog to which this client writes.
   * @param settings The client settings used to control the behavior
   * of the client instance.
   */
  VersionedLayerClient(client::HRN catalog, client::OlpClientSettings settings);

  /**
   * @brief Starts the batch operation.
   *
   * @param request The `StartBatchRequest` object.
   *
   * @return `CancellableFuture` that contains `StartBatchResponse`.
   */
  olp::client::CancellableFuture<StartBatchResponse> StartBatch(
      model::StartBatchRequest request);

  /**
   * @brief Starts the batch operation.
   *
   * @param request The `StartBatchRequest` object.
   * @param callback `StartBatchCallback` that is called with
   * `StartBatchResponse` when the operation completes.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken StartBatch(model::StartBatchRequest request,
                                            StartBatchCallback callback);

  /**
   * @brief Gets the latest version number of the catalog.
   *
   * @return `CancellableFuture` that contains the latest version number
   * of the catalog.
   */
  olp::client::CancellableFuture<GetBaseVersionResponse> GetBaseVersion();

  /**
   * @brief Gets the latest version number of the catalog.
   *
   * @param callback `GetBaseVersionCallback` that is called with
   * `GetBaseVersionResponse` when the operation completes.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken GetBaseVersion(
      GetBaseVersionCallback callback);

  /**
   * @brief Gets the details of the batch publication.
   *
   * @param pub The `Publication` instance.
   *
   * @return `CancellableFuture` that contains the details of the batch
   * publication.
   */
  olp::client::CancellableFuture<GetBatchResponse> GetBatch(
      const model::Publication& pub);

  /**
   * @brief Gets the details of the batch publication.
   *
   * @param pub The `Publication` instance.
   * @param callback `GetBatchCallback` that is called with
   * `GetBatchResponse` when the operation completes.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken GetBatch(const model::Publication& pub,
                                          GetBatchCallback callback);

  /**
   * @brief Completes the batch operation and commits it to the HERE platform.
   *
   * @param pub The `Publication` instance.
   *
   * @return `CancellableFuture` that contains `CompleteBatchResponse`.
   */
  olp::client::CancellableFuture<CompleteBatchResponse> CompleteBatch(
      const model::Publication& pub);

  /**
   * @brief Completes the batch operation and commits it to the HERE platform.
   *
   * @param pub The `Publication` instance.
   * @param callback `CompleteBatchCallback` that is called with
   * `CompleteBatchResponse` when the operation completes.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken CompleteBatch(const model::Publication& pub,
                                               CompleteBatchCallback callback);

  /**
   * @brief Cancels the batch operation
   *
   * @param pub The `Publication` instance.
   *
   * @return CancellableFuture` that contains `CancelBatchResponse`.
   */
  olp::client::CancellableFuture<CancelBatchResponse> CancelBatch(
      const model::Publication& pub);

  /**
   * @brief Cancels the batch operation.
   *
   * @param pub The `Publication` instance.
   * @param callback `CancelBatchCallback` that is called with
   * `CancelBatchResponse` when the operation is cancelled.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken CancelBatch(const model::Publication& pub,
                                             CancelBatchCallback callback);

  /**
   * @brief Cancels all the ongoing operations that this client started.
   *
   * Returns instantly and does not wait for callbacks.
   * Use this operation to cancel all the pending requests without
   * destroying the actual client instance.
   */
  void CancelPendingRequests();

  /**
   * @brief Publishes data to the versioned layer.
   *
   * @note The content-type for this request is set implicitly based on
   * the layer metadata of the target layer.
   *
   * @param pub The `Publication` instance.
   * @param request The `PublishPartitionDataRequest` object.
   *
   * @return `CancellableFuture` that contains `PublishPartitionDataResponse`.
   */
  olp::client::CancellableFuture<PublishPartitionDataResponse> PublishToBatch(
      const model::Publication& pub,
      model::PublishPartitionDataRequest request);

  /**
   * @brief Publishes data to the versioned layer.
   *
   * @note The content-type for this request is set implicitly based on
   * the layer metadata of the target layer.
   *
   * @param pub The `Publication` instance.
   * @param request The `PublishPartitionDataRequest` object.
   * @param callback `PublishPartitionDataCallback` that is called with
   * `PublishPartitionDataResponse` when the operation completes.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken PublishToBatch(
      const model::Publication& pub, model::PublishPartitionDataRequest request,
      PublishPartitionDataCallback callback);

  /**
   * @brief Checks whether the data handle exits.
   *
   * @param request The `CheckDataExistsRequest` object.
   *
   * @return `CancellableFuture` that contains `CheckDataExistsResponse`.
   */
  olp::client::CancellableFuture<CheckDataExistsResponse> CheckDataExists(
      model::CheckDataExistsRequest request);

  /**
   * @brief Checks whether the data handle exits.
   *
   * @param request The `CheckDataExistsRequest` object.
   * @param callback `CheckDataExistsCallback` that is called with
   * `CheckDataExistsResponse` when the operation completes.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken CheckDataExists(
      model::CheckDataExistsRequest request, CheckDataExistsCallback callback);

 private:
  std::shared_ptr<VersionedLayerClientImpl> impl_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
