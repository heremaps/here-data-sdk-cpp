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

namespace olp {
namespace dataservice {
namespace write {

using PutBlobResponse =
    client::ApiResponse<client::ApiNoResult, client::ApiError>;
using PutBlobCallback = std::function<void(PutBlobResponse)>;
using DeleteBlobRespone =
    client::ApiResponse<client::ApiNoResult, client::ApiError>;
using DeleteBlobCallback = std::function<void(DeleteBlobRespone)>;
using CheckBlobRespone = client::ApiResponse<int, client::ApiError>;
using CheckBlobCallback = std::function<void(CheckBlobRespone)>;

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
class BlobApi {
 public:
  /**
   * @brief Publishes a data blob
   * Persists the data blob in the underlying storage mechanism (volume). Use
   * this upload mechanism for blobs smaller than 50 MB. The size limit for
   * blobs uploaded this way is 5 GB but we do not recommend uploading blobs
   * this large with this method. When the operation completes successfully
   * there is no guarantee that the data blob will be immediately available
   * although in most cases it will be. To check if the data blob is available
   * use the HEAD method.
   * @param client Instance of OlpClient used to make REST request.
   * @param layer_id The ID of the layer that the data blob belongs to.
   * @param content_type The content type configured for the target layer.
   * @param data_handle The data handle (ID) represents an identifier for the
   * data blob.
   * @param data Content to be uploaded to OLP.
   * @param billing_tag Optional. An optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   *
   * @return A CancellableFuture containing the PutBlobResponse.
   */
  static client::CancellableFuture<PutBlobResponse> PutBlob(
      const client::OlpClient& client, const std::string& layer_id,
      const std::string& content_type, const std::string& data_handle,
      const std::shared_ptr<std::vector<unsigned char>>& data,
      const boost::optional<std::string>& billing_tag = boost::none);

  /**
   * @brief Publishes a data blob
   * Persists the data blob in the underlying storage mechanism (volume). Use
   * this upload mechanism for blobs smaller than 50 MB. The size limit for
   * blobs uploaded this way is 5 GB but we do not recommend uploading blobs
   * this large with this method. When the operation completes successfully
   * there is no guarantee that the data blob will be immediately available
   * although in most cases it will be. To check if the data blob is available
   * use the HEAD method.
   * @param client Instance of OlpClient used to make REST request.
   * @param layer_id The ID of the layer that the data blob belongs to.
   * @param content_type The content type configured for the target layer.
   * @param data_handle The data handle (ID) represents an identifier for the
   * data blob.
   * @param data Content to be uploaded to OLP.
   * @param billing_tag Optional. An optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   * @param callback PutBlobCallback which will be called with the
   * PutBlobResponse when the operation completes.
   *
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  static client::CancellationToken PutBlob(
      const client::OlpClient& client, const std::string& layer_id,
      const std::string& content_type, const std::string& data_handle,
      const std::shared_ptr<std::vector<unsigned char>>& data,
      const boost::optional<std::string>& billing_tag,
      PutBlobCallback callback);

  /**
   * @brief Delete a data blob
   * Deletes a data blob from the underlying storage mechanism (volume). When
   * you delete a blob, you cannot upload data to the deleted blob's data handle
   * for at least 3 days. The DELETE method works only for index layers. DELETE
   * requests for blobs stored for other kind of layers will be rejected.
   * @param client Instance of OlpClient used to make REST request.
   * @param layer_id The ID of the layer that the data blob belongs to.
   * @param data_handle The data handle (ID) represents an identifier for the
   * data blob.
   * @param billing_tag Optional. An optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   * @param callback DeleteBlobCallback which will be called with the
   * DeleteBlobResponse when the operation completes.
   * @return
   */
  static client::CancellationToken deleteBlob(
      const client::OlpClient& client, const std::string& layer_id,
      const std::string& data_handle,
      const boost::optional<std::string>& billing_tag,
      const DeleteBlobCallback& callback);

  /**
   * @brief Checks if a data handle exists
   * Checks if a blob exists for the requested data handle.
   * @param client Instance of OlpClient used to make REST request.
   * @param layer_id The ID of the layer that the data blob belongs to.
   * @param data_handle The data handle (ID) represents an identifier for the
   * data blob.
   * @param billing_tag Optional. An optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   * @param callback CheckBlobCallback which will be called with the
   * CheckBlobResponse when the operation completes.
   * @return
   */
  static client::CancellationToken checkBlobExists(
      const client::OlpClient& client, const std::string& layer_id,
      const std::string& data_handle,
      const boost::optional<std::string>& billing_tag,
      const CheckBlobCallback& callback);
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
