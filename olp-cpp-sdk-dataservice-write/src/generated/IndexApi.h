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
#include <string>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiNoResult.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/OlpClient.h>

#include <olp/dataservice/write/generated/model/Index.h>
#include <olp/dataservice/write/model/UpdateIndexRequest.h>

namespace olp {
namespace dataservice {
namespace write {

using InsertIndexesResponse =
    client::ApiResponse<client::ApiNoResult, client::ApiError>;
using InsertIndexesCallback = std::function<void(InsertIndexesResponse)>;

using UpdateIndexesResponse =
    client::ApiResponse<client::ApiNoResult, client::ApiError>;
using UpdateIndexesCallback = std::function<void(UpdateIndexesResponse)>;

/**
 * @brief The blob service supports the upload and retrieval of large volumes of
 * data from the storage of a catalog. Each discrete chunk of data is stored as
 * a blob (Binary Large Object). Each blob has its own unique ID (data handle)
 * which is stored as partition metadata. To get a partition's data, you first
 * use the metadata service to retrieve the partition's metadata with the data
 * handle of the relevant blobs. You then use the data handle to pull the data
 * using the blob service. If you are writing to a volatile layer, see
 * volatile-blob API definition.
 */
class IndexApi {
 public:
  /**
   * @brief Inserts index data to an index layer
   * Adds index data for a given data blob to an index layer.
   * For more information, see [Publish to an Index Layer]
   * (https://developer.here.com/olp/documentation/data-store/data_dev_guide/rest/publishing-data-index.html).
   * @param client Instance of OlpClient used to make REST request.
   * @param indexes An array of index attributes and values to be inserted.
   * @param layerID The layer ID of the index layer.
   * @param billing_tag Optional. An optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   *
   * @return A CancellableFuture containing the InsertIndexesResponse.
   */
  static client::CancellableFuture<InsertIndexesResponse> insertIndexes(
      const client::OlpClient& client, const model::Index& indexes,
      const std::string& layer_id,
      const boost::optional<std::string>& billing_tag = boost::none);

  /**
   * @brief Inserts index data to an index layer
   * Adds index data for a given data blob to an index layer.
   * For more information, see [Publish to an Index Layer]
   * (https://developer.here.com/olp/documentation/data-store/data_dev_guide/rest/publishing-data-index.html).
   * @param client Instance of OlpClient used to make REST request.
   * @param indexes An array of index attributes and values to be inserted.
   * @param layerID The layer ID of the index layer.
   * @param billing_tag Optional. An optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   * @param callback PutBlobCallback which will be called with the
   * PutBlobResponse when the operation completes.
   *
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  static client::CancellationToken insertIndexes(
      const client::OlpClient& client, const model::Index& indexes,
      const std::string& layer_id,
      const boost::optional<std::string>& billing_tag,
      InsertIndexesCallback callback);
  /**
   * @brief Updates index layer partitions
   * Modifies partitions in an index layer.
   * @param client Instance of OlpClient used to make REST request.
   * @param additions An array of index attributes and values to be added.
   * @param removals An array of dataHandles to be removed.
   * @param layerID The layer ID of the index layer.
   * @param billing_tag Optional. An optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   *
   * @return A CancellableFuture containing the InsertIndexesResponse.
   */
  static client::CancellableFuture<UpdateIndexesResponse> performUpdate(
      const client::OlpClient& client, const model::UpdateIndexRequest& request,
      const boost::optional<std::string>& billing_tag = boost::none);

  /**
   * @brief Updates index layer partitions
   * Modifies partitions in an index layer.
   * @param client Instance of OlpClient used to make REST request.
   * @param additions An array of index attributes and values to be added.
   * @param removals An array of dataHandles to be removed.
   * @param layerID The layer ID of the index layer.
   * @param billing_tag Optional. An optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   * @param callback PutBlobCallback which will be called with the
   * PutBlobResponse when the operation completes.
   *
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  static client::CancellationToken performUpdate(
      const client::OlpClient& client, const model::UpdateIndexRequest& request,
      const boost::optional<std::string>& billing_tag,
      UpdateIndexesCallback callback);
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
