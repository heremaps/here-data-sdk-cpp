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

/**
 * @brief Client responsible for writing data into a HERE platform volatile
 * layer.
 */
class DATASERVICE_WRITE_API VolatileLayerClient {
 public:
  /**
   * @brief VolatileLayerClient Constructor.
   * @param catalog The HRN specifying the catalog this client will write to.
   * @param settings Client settings used to control behaviour of the client
   * instance. Volatile.
   */
  VolatileLayerClient(client::HRN catalog, client::OlpClientSettings settings);

  /**
   * @brief Cancels all the ongoing operations that this client started.
   *
   * Returns instantly and does not wait for the callbacks.
   * Use this operation to cancel all the pending requests without
   * destroying the actual client instance.
   */
  void CancelPendingRequests();

  /**
   * @brief Call to publish data into a volatile layer.
   * @note Content-type for this request will be set implicitly based on the
   * layer metadata for the target layer on the HERE platform.
   * @param request PublishPartitionDataRequest object representing the
   * parameters for this publishData call.
   *
   * @return A CancellableFuture containing the PublishPartitionDataResponse.
   */
  olp::client::CancellableFuture<PublishPartitionDataResponse>
  PublishPartitionData(model::PublishPartitionDataRequest request);

  /**
   * @brief Call to publish data into a volatile layer.
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
  olp::client::CancellationToken PublishPartitionData(
      model::PublishPartitionDataRequest request,
      PublishPartitionDataCallback callback);

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
   * @brief Publish meta data to the HERE platform.
   *
   * A volatile layer publishing task consists 2 steps:
   * <list type = "number">
   * <item>Publish meta data</item>
   * <item>Publish data blob</item>
   * </list>
   *
   * This API handles the 1st step, it has to be done <b>before</b> publishing
   * data blob, otherwise clients will receive an empty vector from publishing
   * data blob result. Note that changing the meta data of a partition will
   * result in updating the catalog version.
   *
   * @param pub The `Publication` instance.
   * @param partitions a group of PublishPartitionDataRequest that has following
   * fields defined: <list type = "bullet"> <item>Layer ID (required)</item>
   * <item>Partition (required)</item>
   * <item>HERE checksum (optional)</item>
   * <item>Data - must NOT be defiend as this call is for updating metadata
   * (e.g. Partition IDs) only.</item>
   * </list>
   *
   * @return cancellation token that can be used to cancel the request.
   */
  olp::client::CancellableFuture<PublishToBatchResponse> PublishToBatch(
      const model::Publication& pub,
      const std::vector<model::PublishPartitionDataRequest>& partitions);

  /**
   * @brief Publish meta data to the HERE platform.
   *
   * A volatile layer publishing task consists 2 steps:
   * <list type = "number">
   * <item>Publish meta data</item>
   * <item>Publish data blob</item>
   * </list>
   *
   * This API handles the 1st step, it has to be done <b>before</b> publishing
   * data blob, otherwise clients will receive an empty vector from publishing
   * data blob result. Note that changing the meta data of a partition will
   * result in updating the catalog version.
   *
   * @param pub The `Publication` instance.
   * @param partitions a group of PublishPartitionDataRequest that has following
   * fields defined: <list type = "bullet"> <item>Layer ID (required)</item>
   * <item>Partition (required)</item>
   * <item>HERE checksum (optional)</item>
   * <item>Data - must NOT be defiend as this call is for updating metadata
   * (e.g. Partition IDs) only.</item>
   * </list>
   * @param callback called when the operation completes.
   *
   * @return cancellation token that can be used to cancel the request.
   */
  olp::client::CancellationToken PublishToBatch(
      const model::Publication& pub,
      const std::vector<model::PublishPartitionDataRequest>& partitions,
      PublishToBatchCallback callback);

  /**
   * @brief Complete the given batch operation and commit to the HERE platform.
   *
   * @param pub The `Publication` instance.
   * @return future containing the batch response
   */
  olp::client::CancellableFuture<CompleteBatchResponse> CompleteBatch(
      const model::Publication& pub);

  /**
   * @brief Complete the given batch operation and commit to the HERE platform.
   *
   * @param pub The `Publication` instance.
   * @param callback called when the operation completes.
   * @return cancellation token
   */
  olp::client::CancellationToken CompleteBatch(const model::Publication& pub,
                                               CompleteBatchCallback callback);

 private:
  std::shared_ptr<VolatileLayerClientImpl> impl_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
