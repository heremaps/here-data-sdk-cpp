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
#include <vector>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiNoResult.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/porting/deprecated.h>

#include <olp/dataservice/write/DataServiceWriteApi.h>
#include <olp/dataservice/write/generated/model/Publication.h>
#include <olp/dataservice/write/generated/model/ResponseOkSingle.h>
#include <olp/dataservice/write/model/PublishPartitionDataRequest.h>
#include <olp/dataservice/write/model/StartBatchRequest.h>
#include <olp/dataservice/write/model/VersionResponse.h>

namespace olp {
namespace client {
struct Error;
class HRN;
}  // namespace client

namespace dataservice {
namespace write {
class VolatileLayerClientImpl;

using PublishPartitionDataResult =
    olp::dataservice::write::model::ResponseOkSingle;
using PublishPartitionDataResponse =
    client::ApiResponse<PublishPartitionDataResult, client::ApiError>;
using PublishPartitionDataCallback =
    std::function<void(PublishPartitionDataResponse response)>;

using GetBaseVersionResult = model::VersionResponse;
using GetBaseVersionResponse =
    client::ApiResponse<GetBaseVersionResult, client::ApiError>;
using GetBaseVersionCallback = std::function<void(GetBaseVersionResponse)>;

using StartBatchResult = model::Publication;
using StartBatchResponse =
    client::ApiResponse<StartBatchResult, client::ApiError>;
using StartBatchCallback = std::function<void(StartBatchResponse)>;

using GetBatchResult = model::Publication;
using GetBatchResponse = client::ApiResponse<GetBatchResult, client::ApiError>;
using GetBatchCallback = std::function<void(GetBatchResponse)>;

using PublishToBatchResult = client::ApiNoResult;
using PublishToBatchResponse =
    client::ApiResponse<PublishToBatchResult, client::ApiError>;
using PublishToBatchCallback =
    std::function<void(PublishToBatchResponse response)>;

using CompleteBatchResult = client::ApiNoResult;
using CompleteBatchResponse =
    client::ApiResponse<CompleteBatchResult, client::ApiError>;
using CompleteBatchCallback =
    std::function<void(CompleteBatchResponse response)>;

/// Publishes data to a volatile layer.
class DATASERVICE_WRITE_API VolatileLayerClient {
 public:
  /**
   * @brief Creates the `VolatileLayerClient` instance.
   *
   * @param catalog The HRN of the catalog to which this client writes.
   * @param settings The client settings used to control the behavior
   * of the client instance.
   */
  VolatileLayerClient(client::HRN catalog, client::OlpClientSettings settings);

  /**
   * @brief Cancels all the ongoing operations that this client started.
   *
   * Returns instantly and does not wait for callbacks.
   * Use this operation to cancel all the pending requests without
   * destroying the actual client instance.
   */
  void CancelPendingRequests();

  /**
   * @brief Publishes data to the volatile layer.
   *
   * @note The content-type for this request is set implicitly based on
   * the layer metadata of the target layer.
   *
   * @param request The `PublishPartitionDataRequest` object.
   *
   * @return `CancellableFuture` that contains `PublishPartitionDataResponse`.
   */
  olp::client::CancellableFuture<PublishPartitionDataResponse>
  PublishPartitionData(model::PublishPartitionDataRequest request);

  /**
   * @brief Publishes data to the volatile layer.
   *
   * @note The content-type for this request is set implicitly based on
   * the layer metadata of the target layer.
   *
   * @param request The `PublishPartitionDataRequest` object.
   * @param callback `PublishPartitionDataCallback` that is called with
   * `PublishPartitionDataResponse` when the operation completes.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken PublishPartitionData(
      model::PublishPartitionDataRequest request,
      PublishPartitionDataCallback callback);

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
   * @brief Publishes metadata to the HERE platform.
   *
   * This task consists of two steps:
   *
   * 1. Publish the metadata.
   * 2. Publish the data blob.
   *
   * This API handles the first step, which has to be done before publishing
   * the data blob. Otherwise, clients will receive an empty vector from
   * the publishing result. Changing the metadata of the partition
   * results in updating the catalog version.
   *
   * @param pub The `Publication` instance.
   * @param partitions A group of `PublishPartitionDataRequest` objects
   * that have the following fields: layer ID, partition, HERE checksum,
   * and data. Do not define the data as this call is only for updating
   * metadata.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellableFuture<PublishToBatchResponse> PublishToBatch(
      const model::Publication& pub,
      const std::vector<model::PublishPartitionDataRequest>& partitions);

  /**
   * @brief Publishes metadata to the HERE platform.
   *
   * This task consists of two steps:
   *
   * 1. Publish the metadata.
   * 2. Publish the data blob.
   *
   * This API handles the first step, which has to be done before publishing
   * the data blob. Otherwise, clients will receive an empty vector from
   * the publishing result. Changing the metadata of the partition
   * results in updating the catalog version.
   *
   * @param pub The `Publication` instance.
   * @param partitions A group of `PublishPartitionDataRequest` objects
   * that have the following fields: layer ID, partition, HERE checksum,
   * and data. Do not define the data as this call is only for updating
   * metadata.
   * @param callback `PublishToBatchCallback that is called with
   * `PublishToBatchResponse` when the operation completes.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken PublishToBatch(
      const model::Publication& pub,
      const std::vector<model::PublishPartitionDataRequest>& partitions,
      PublishToBatchCallback callback);

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

 private:
  std::shared_ptr<VolatileLayerClientImpl> impl_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
