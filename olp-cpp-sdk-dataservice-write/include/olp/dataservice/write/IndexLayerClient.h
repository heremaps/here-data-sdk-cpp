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
#include <olp/core/client/ApiNoResult.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/OlpClientSettings.h>
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
/**
 * @brief Client responsible for writing data into an OLP Index Layer.
 */
class DATASERVICE_WRITE_API IndexLayerClient {
 public:
  /**
   * @brief IndexLayerClient Constructor.
   * @param catalog OLP HRN specifying the catalog this client will write to.
   * @param settings Client settings used to control behaviour of the client
   * instance. Index.
   */
  IndexLayerClient(const client::HRN& catalog,
                   const client::OlpClientSettings& settings);

  /**
   * @brief Cancel all pending requests.
   */
  void CancelAll();

  /**
   * @brief Call to publish index into an OLP Index Layer.
   * @note Content-type for this request will be set implicitly based on the
   * Layer metadata for the target Layer on OLP.
   * @param request PublishIndexRequest object representing the
   * parameters for this publishIndex call.
   *
   * @return A CancellableFuture containing the PublishIndexResponse.
   */
  olp::client::CancellableFuture<PublishIndexResponse> PublishIndex(
      const model::PublishIndexRequest& request);

  /**
   * @brief Call to publish index into an OLP Index Layer.
   * @note Content-type for this request will be set implicitly based on the
   * Layer metadata for the target Layer on OLP.
   * @param request PublishIndexRequest object representing the
   * parameters for this publishIndex call.
   * @param callback PublishIndexCallback which will be called with the
   * PublishIndexResponse when the operation completes.
   *
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken PublishIndex(
      const model::PublishIndexRequest& request,
      const PublishIndexCallback& callback);

  /**
   * @brief Call to delete data blob that is stored under index
   * layer.
   * @param request DeleteIndexDataRequest object representing the parameters
   * for deleteIndexData call.
   * @param callback DeleteIndexDataCallback which will be called with
   * DeleteIndexDataResponse when operation completes.
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken DeleteIndexData(
      const model::DeleteIndexDataRequest& request,
      const DeleteIndexDataCallback& callback);

  /**
   * @brief Call to delete data blob that is stored under index
   * layer.
   * @param request DeleteIndexDataRequest object representing the parameters
   * for deleteIndexData call.
   * @return A CancellableFuture containing the DeleteIndexDataResponse.
   */
  olp::client::CancellableFuture<DeleteIndexDataResponse> DeleteIndexData(
      const model::DeleteIndexDataRequest& request);

  /**
   * @brief Call to update index information to an OLP Index Layer.
   * @param request UpdateIndexRequest object representing the
   * parameters for this updateIndex call.
   *
   * @return A CancellableFuture containing the PublishIndexResponse.
   */
  olp::client::CancellableFuture<UpdateIndexResponse> UpdateIndex(
      const model::UpdateIndexRequest& request);

  /**
   * @brief Call to update index information to an OLP Index Layer
   * @param request PublishIndexRequest object representing the
   * parameters for this publishIndex call.
   * @param callback PublishIndexCallback which will be called with the
   * PublishIndexResponse when the operation completes.
   *
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken UpdateIndex(
      const model::UpdateIndexRequest& request,
      const UpdateIndexCallback& callback);

 private:
  std::shared_ptr<IndexLayerClientImpl> impl_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
