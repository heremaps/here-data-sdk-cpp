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

#include <vector>

#include <boost/optional.hpp>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>

namespace olp {
namespace client {
class OlpClient;
class CancellationContext;
}  // namespace client

namespace dataservice {
namespace write {
namespace model {
class ResponseOk;
class ResponseOkSingle;
}  // namespace model

using IngestDataResponse =
    client::ApiResponse<model::ResponseOkSingle, client::ApiError>;
using IngestDataCallback = std::function<void(IngestDataResponse)>;

using IngestSdiiResponse =
    client::ApiResponse<model::ResponseOk, client::ApiError>;

/**
 * @brief API responsible for ingesting data into an OLP Stream Layer.
 */
class IngestApi {
 public:
  /**
   * @brief Call to ingest data into an OLP Stream Layer.
   * @param client Instance of OlpClient used to make REST request.
   * @param layer_id Layer of the catalog where you want to store the data. The
   * layer type must be Stream.
   * @param content_type The content type configured for the target layer.
   * @param data Content to be uploaded to OLP.
   * @param trace_id Optional. A unique message ID, such as a UUID. This can be
   * included in the request if you want to use an ID that you define. If you do
   * not include an ID, one will be generated during ingestion and included in
   * the response. You can use this ID to track your request and identify the
   * message in the catalog.
   * @param billing_tag Optional. An optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   * @param checksum A SHA-256 hash you can provide for
   * validation against the calculated value on the request body hash. This
   * verifies the integrity of your request and prevents modification by a third
   * party.It will be created by the service if not provided. A SHA-256 hash
   * consists of 256 bits or 64 chars.
   * @param callback IngestDataCallback which will be called with the
   * IngestDataResponse when the operation completes.
   *
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  static client::CancellationToken IngestData(
      const client::OlpClient& client, const std::string& layer_id,
      const std::string& content_type,
      const std::shared_ptr<std::vector<unsigned char>>& data,
      const boost::optional<std::string>& trace_id,
      const boost::optional<std::string>& billing_tag,
      const boost::optional<std::string>& checksum,
      IngestDataCallback callback);

  /**
   * @brief Send list of SDII messages to a stream layer.
   * SDII message data must be in SDII MessageList protobuf format. For more
   * information please see the OLP Sensor Data Ingestion Interface
   * documentation and schemas.
   * @note the Content-Type for this request is always "application/x-protobuf".
   * @param client Instance of OlpClient used to make REST request.
   * @param layer_id Layer of the catalog where you want to store the data.
   * @param sdii_message_list SDII MessageList data encoded in protobuf format
   * according to the OLP SDII Message List schema. The maximum size is 20 MB.
   * @param trace_id Optional. A unique message ID, such as a UUID. This can be
   * included in the request if you want to use an ID that you define. If you do
   * not include an ID, one will be generated during ingestion and included in
   * the response. You can use this ID to track your request and identify the
   * message in the catalog.
   * @param billing_tag Optional. An optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   * @param checksum A SHA-256 hash you can provide for
   * validation against the calculated value on the request body hash. This
   * verifies the integrity of your request and prevents modification by a third
   * party.It will be created by the service if not provided. A SHA-256 hash
   * consists of 256 bits or 64 chars.
   * @param context A CancellationContext, which can be used to cancel request.
   *
   * @return A IngestSdiiResponse which contains error or ResponseOk
   */
  static IngestSdiiResponse IngestSdii(
      const client::OlpClient& client, const std::string& layer_id,
      const std::shared_ptr<std::vector<unsigned char>>& sdii_message_list,
      const boost::optional<std::string>& trace_id,
      const boost::optional<std::string>& billing_tag,
      const boost::optional<std::string>& checksum,
      client::CancellationContext context);
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
