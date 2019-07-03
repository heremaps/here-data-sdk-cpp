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

#include <generated/model/PublishPartitions.h>
#include <olp/dataservice/write/generated/model/Publication.h>

namespace olp {
namespace dataservice {
namespace write {

using InitPublicationResponse =
    client::ApiResponse<model::Publication, client::ApiError>;
using InitPublicationCallback = std::function<void(InitPublicationResponse)>;

using UploadPartitionsResponse =
    client::ApiResponse<client::ApiNoResult, client::ApiError>;
using UploadPartitionsCallback = std::function<void(UploadPartitionsResponse)>;

using SubmitPublicationResponse =
    client::ApiResponse<client::ApiNoResult, client::ApiError>;
using SubmitPublicationCallback =
    std::function<void(SubmitPublicationResponse)>;

using GetPublicationResponse =
    client::ApiResponse<model::Publication, client::ApiError>;
using GetPublicationCallback = std::function<void(GetPublicationResponse)>;

using CancelPublicationResponse =
    client::ApiResponse<client::ApiNoResult, client::ApiError>;
using CancelPublicationCallback =
    std::function<void(CancelPublicationResponse)>;

/**
 * @brief Manage the publishing of data to a catalog. Supports publish to
 * versioned, volatile and stream layer types. See Data API Developer’s Guide in
 * the Documentation section for more information.
 */
class PublishApi {
 public:
  /**
   * @brief Initialize a new publication
   * Initializes a new publication for publishing metadata. Determines the
   * publication type based on the provided layer IDs. A publication can only
   * consist of layer IDs that have the same layer type. For example, you can
   * have a publication for multiple layers of type `versioned`, but you cannot
   * have a single publication that publishes to both `versioned` and `stream`
   * layers. In addition, you may only have one `versioned` or `volatile`
   * publication in process at a time. You cannot have multiple active
   * publications to the same catalog for `versioned` and `volatile` layer
   * types. The body field `versionDependencies` is optional and is used for
   * `versioned` layers to declare version dependencies.
   * @param client Instance of OlpClient used to make REST request.
   * @param publication Fields to initialize a publication.
   * @param billing_tag Optional. An optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   * @return A CancellableFuture containing the InitPublicationResponse.
   */
  static client::CancellableFuture<InitPublicationResponse> InitPublication(
      const client::OlpClient& client, const model::Publication& publication,
      const boost::optional<std::string>& billing_tag = boost::none);

  /**
   * @brief Initialize a new publication
   * Initializes a new publication for publishing metadata. Determines the
   * publication type based on the provided layer IDs. A publication can only
   * consist of layer IDs that have the same layer type. For example, you can
   * have a publication for multiple layers of type `versioned`, but you cannot
   * have a single publication that publishes to both `versioned` and `stream`
   * layers. In addition, you may only have one `versioned` or `volatile`
   * publication in process at a time. You cannot have multiple active
   * publications to the same catalog for `versioned` and `volatile` layer
   * types. The body field `versionDependencies` is optional and is used for
   * `versioned` layers to declare version dependencies.
   * @param client Instance of OlpClient used to make REST request.
   * @param publication Fields to initialize a publication.
   * @param billing_tag Optional. An optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   * @param callback InitPublicationCallback which will be called with the
   * InitPublicationResponse when the operation completes.
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  static client::CancellationToken InitPublication(
      const client::OlpClient& client, const model::Publication& publication,
      const boost::optional<std::string>& billing_tag,
      InitPublicationCallback callback);

  /**
   * @brief Upload partitions
   * Upload partitions to the given layer. Dependending on the publication type,
   * post processing may be required before the partitions are published. For
   * better performance batch your partitions (e.g. 1000 per request), rather
   * than uploading them individually.
   * @param client Instance of OlpClient used to make REST request.
   * @param publish_partitions Publication partitions. Data and DataHandle
   * fields cannot be populated at the same time. See Partition object for more
   * details on body fields
   * @param publication_id The ID of publication to publish to.
   * @param layer_id The ID of the layer to publish to.
   * @param billing_tag Billing Tag is an optional free-form tag which is used
   * for grouping billing records together. If supplied, it must be between 4 -
   * 16 characters and contain only alphanumeric ASCII characters [A-Za-z0-9].
   * Grouping billing records by billing tag will be available in future
   * releases.
   * @return A CancellableFuture containing the UploadPartitionsResponse.
   */
  static client::CancellableFuture<UploadPartitionsResponse> UploadPartitions(
      const client::OlpClient& client,
      const model::PublishPartitions& publish_partitions,
      const std::string& publication_id, const std::string& layer_id,
      const boost::optional<std::string>& billing_tag = boost::none);

  /**
   * @brief Upload partitions
   * Upload partitions to the given layer. Dependending on the publication type,
   * post processing may be required before the partitions are published. For
   * better performance batch your partitions (e.g. 1000 per request), rather
   * than uploading them individually.
   * @param client Instance of OlpClient used to make REST request.
   * @param publish_partitions Publication partitions. Data and DataHandle
   * fields cannot be populated at the same time. See Partition object for more
   * details on body fields
   * @param publication_id The ID of publication to publish to.
   * @param layer_id The ID of the layer to publish to.
   * @param billing_tag Billing Tag is an optional free-form tag which is used
   * for grouping billing records together. If supplied, it must be between 4 -
   * 16 characters and contain only alphanumeric ASCII characters [A-Za-z0-9].
   * Grouping billing records by billing tag will be available in future
   * releases.
   * @param callback UploadPartitionsCallback which will be called with the
   * UploadPartitionsResponse when the operation completes.
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  static client::CancellationToken UploadPartitions(
      const client::OlpClient& client,
      const model::PublishPartitions& publish_partitions,
      const std::string& publication_id, const std::string& layer_id,
      const boost::optional<std::string>& billing_tag,
      UploadPartitionsCallback callback);

  /**
   * @brief Submits a publication
   * Submits the publication and initiates post processing if necessary.
   * Publication state becomes `Submitted` directly after submission and
   * `Succeeded` after successful processing. See Data API Developer’s Guide in
   * the Documentation section for complete publication states diagram.
   * @param client Instance of OlpClient used to make REST request.
   * @param publication_id ID of publication to submit.
   * @param billing_tag Billing Tag is an optional free-form tag which is used
   * for grouping billing records together. If supplied, it must be between 4 -
   * 16 characters and contain only alphanumeric ASCII characters [A-Za-z0-9].
   * Grouping billing records by billing tag will be available in future
   * releases.
   *
   * @return A CancellableFuture containing the SubmitPublicationResponse.
   */
  static client::CancellableFuture<SubmitPublicationResponse> SubmitPublication(
      const client::OlpClient& client, const std::string& publication_id,
      const boost::optional<std::string>& billing_tag = boost::none);

  /**
   * @brief Submits a publication
   * Submits the publication and initiates post processing if necessary.
   * Publication state becomes `Submitted` directly after submission and
   * `Succeeded` after successful processing. See Data API Developer’s Guide in
   * the Documentation section for complete publication states diagram.
   * @param client Instance of OlpClient used to make REST request.
   * @param publication_id ID of publication to submit.
   * @param billing_tag Billing Tag is an optional free-form tag which is used
   * for grouping billing records together. If supplied, it must be between 4 -
   * 16 characters and contain only alphanumeric ASCII characters [A-Za-z0-9].
   * Grouping billing records by billing tag will be available in future
   * releases.
   * @param client Instance of OlpClient used to make REST request.
   * @param callback SubmitPublicationCallback which will be called with the
   * SubmitPublicationResponse when the operation completes.
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  static client::CancellationToken SubmitPublication(
      const client::OlpClient& client, const std::string& publication_id,
      const boost::optional<std::string>& billing_tag,
      SubmitPublicationCallback callback);

  /**
   * @brief Gets a publication
   * Returns the details of the specified publication. Publication can be in one
   * of these states: Initialized, Submitted, Cancelled, Failed, Succeeded,
   * Expired. See Data API Developer’s Guide in the Documentation section for
   * the publication state diagram.
   * @param client Instance of OlpClient used to make REST request.
   * @param publication_id The ID of the publication to retrieve.
   * @param billing_tag Billing Tag is an optional free-form tag which is used
   * for grouping billing records together. If supplied, it must be between 4 -
   * 16 characters and contain only alphanumeric ASCII characters [A-Za-z0-9].
   * Grouping billing records by billing tag will be available in future
   * releases.
   *
   * @return A CancellableFuture containing the GetPublicationResponse.
   */
  static client::CancellableFuture<GetPublicationResponse> GetPublication(
      const client::OlpClient& client, const std::string& publication_id,
      const boost::optional<std::string>& billing_tag = boost::none);

  /**
   * @brief Gets a publication
   * Returns the details of the specified publication. Publication can be in one
   * of these states: Initialized, Submitted, Cancelled, Failed, Succeeded,
   * Expired. See Data API Developer’s Guide in the Documentation section for
   * the publication state diagram.
   * @param client Instance of OlpClient used to make REST request.
   * @param publication_id The ID of the publication to retrieve.
   * @param billing_tag Billing Tag is an optional free-form tag which is used
   * for grouping billing records together. If supplied, it must be between 4 -
   * 16 characters and contain only alphanumeric ASCII characters [A-Za-z0-9].
   * Grouping billing records by billing tag will be available in future
   * releases.
   * @param callback GetPublicationCallback which will be called with the
   * GetPublicationResponse when the operation completes.
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  static client::CancellationToken GetPublication(
      const client::OlpClient& client, const std::string& publication_id,
      const boost::optional<std::string>& billing_tag,
      GetPublicationCallback callback);

  /**
   * @brief Cancels a publication
   * Cancels the publication.
   * Publication state becomes `Cancelled`.
   * @param client Instance of OlpClient used to make REST request.
   * @param publication_id ID of publication to submit.
   * @param billing_tag Billing Tag is an optional free-form tag which is used
   * for grouping billing records together. If supplied, it must be between 4 -
   * 16 characters and contain only alphanumeric ASCII characters [A-Za-z0-9].
   * Grouping billing records by billing tag will be available in future
   * releases.
   *
   * @return A CancellableFuture containing the CancelPublicationResponse.
   */
  static client::CancellableFuture<CancelPublicationResponse> CancelPublication(
      const client::OlpClient& client, const std::string& publication_id,
      const boost::optional<std::string>& billing_tag = boost::none);

  /**
   * @brief Cancels a publication
   * Cancels the publication
   * @param client Instance of OlpClient used to make REST request.
   * @param publication_id ID of publication to submit.
   * @param billing_tag Billing Tag is an optional free-form tag which is used
   * for grouping billing records together. If supplied, it must be between 4 -
   * 16 characters and contain only alphanumeric ASCII characters [A-Za-z0-9].
   * Grouping billing records by billing tag will be available in future
   * releases.
   * @param client Instance of OlpClient used to make REST request.
   * @param callback CancelPublicationCallback which will be called with the
   * CancelPublicationResponse when the operation completes.
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  static client::CancellationToken CancelPublication(
      const client::OlpClient& client, const std::string& publication_id,
      const boost::optional<std::string>& billing_tag,
      CancelPublicationCallback callback);
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
