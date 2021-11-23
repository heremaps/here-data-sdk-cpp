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
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/porting/deprecated.h>
#include <olp/dataservice/write/DataServiceWriteApi.h>
#include <olp/dataservice/write/generated/model/ResponseOkSingle.h>
#include <olp/dataservice/write/model/DeleteIndexDataRequest.h>
#include <olp/dataservice/write/model/PublishIndexRequest.h>
#include <olp/dataservice/write/model/UpdateIndexRequest.h>

namespace olp {
namespace client {
struct Error;
class HRN;
}  // namespace client

namespace dataservice {
namespace write {
class IndexLayerClientImpl;

using PublishIndexResult = olp::dataservice::write::model::ResponseOkSingle;
using PublishIndexResponse =
    client::ApiResponse<PublishIndexResult, client::ApiError>;
using PublishIndexCallback = std::function<void(PublishIndexResponse response)>;

using DeleteIndexDataResponse =
    client::ApiResponse<client::ApiNoResult, client::ApiError>;
using DeleteIndexDataCallback =
    std::function<void(DeleteIndexDataResponse response)>;

using UpdateIndexResponse =
    client::ApiResponse<client::ApiNoResult, client::ApiError>;
using UpdateIndexCallback = std::function<void(UpdateIndexResponse response)>;

/// Publishes data to an index layer.
class DATASERVICE_WRITE_API IndexLayerClient {
 public:
  /**
   * @brief Creates the `IndexLayerClient` instance.
   *
   * @param catalog The HRN of the catalog to which this client writes.
   * @param settings The client settings used to control the behavior
   * of the client instance.
   */
  IndexLayerClient(client::HRN catalog, client::OlpClientSettings settings);

  /**
   * @brief Cancels all the ongoing operations that this client started.
   *
   * It returns instantly and does not wait for callbacks.
   * Use this operation to cancel all the pending requests without
   * destroying the actual client instance.
   */
  void CancelPendingRequests();

  /**
   * @brief Publishes the index to the index layer.
   *
   * @note The content-type for this request is set implicitly based on
   * the layer metadata of the target layer.
   *
   * @param request The `PublishIndexRequest` object.
   *
   * @return `CancellableFuture` that contains `PublishIndexResponse`.
   */
  olp::client::CancellableFuture<PublishIndexResponse> PublishIndex(
      model::PublishIndexRequest request);

  /**
   * @brief Publishes the index to the index layer.
   *
   * @note The content-type for this request is set implicitly based on
   * the layer metadata of the target layer.
   *
   * @param request The `PublishIndexRequest` object.
   * @param callback `PublishIndexCallback` that is called with
   * `PublishIndexResponse` when the operation completes.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken PublishIndex(
      model::PublishIndexRequest request, PublishIndexCallback callback);

  /**
   * @brief Deletes the data blob that is stored under the index
   * layer.
   *
   * @param request The `DeleteIndexDataRequest` object.
   * @param callback `DeleteIndexDataCallback` that is called with
   * `DeleteIndexDataResponse` when the operation completes.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken DeleteIndexData(
      model::DeleteIndexDataRequest request, DeleteIndexDataCallback callback);

  /**
   * @brief Deletes the data blob that is stored under the index
   * layer.
   *
   * @param request The `DeleteIndexDataRequest` object.
   *
   * @return `CancellableFuture` that contains `DeleteIndexDataResponse`.
   */
  olp::client::CancellableFuture<DeleteIndexDataResponse> DeleteIndexData(
      model::DeleteIndexDataRequest request);

  /**
   * @brief Updates index information in the index layer.
   *
   * @param request The `UpdateIndexRequest` object.
   *
   * @return `CancellableFuture` that contains `PublishIndexResponse`.
   */
  olp::client::CancellableFuture<UpdateIndexResponse> UpdateIndex(
      model::UpdateIndexRequest request);

  /**
   * @brief Updates index information in the index layer.
   *
   * @param request The `UpdateIndexRequest` object.
   * @param callback `PublishIndexCallback` that is called with
   * `PublishIndexResponse` when the operation completes.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken UpdateIndex(model::UpdateIndexRequest request,
                                             UpdateIndexCallback callback);

 private:
  std::shared_ptr<IndexLayerClientImpl> impl_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
