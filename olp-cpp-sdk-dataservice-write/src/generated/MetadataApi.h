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
#include <olp/core/client/ApiResponse.h>
#include <boost/optional.hpp>
#include "generated/model/LayerVersions.h"
#include "generated/model/Partitions.h"
#include "olp/dataservice/write/model/VersionResponse.h"

namespace olp {
namespace client {
class OlpClient;
}

namespace dataservice {
namespace write {
/**
 * @brief Api to get information about catalogs, layers, and partitions.
 */
class MetadataApi {
 public:
  using PartitionsResponse =
      client::ApiResponse<model::Partitions, client::ApiError>;
  using PartitionsCallback = std::function<void(PartitionsResponse)>;

  using CatalogVersionResponse =
      client::ApiResponse<model::VersionResponse, client::ApiError>;
  using CatalogVersionCallback = std::function<void(CatalogVersionResponse)>;

  using LayerVersionsResponse =
      client::ApiResponse<model::LayerVersions, client::ApiError>;
  using LayerVersionsCallback = std::function<void(LayerVersionsResponse)>;

  /**
   * @brief Call to asynchronously retrieve the latest metadata version for each
   * layer of a specified catalog metadata version.
   * @param client Instance of OlpClient used to make REST request.
   * @param version The catalog version.
   * @param billingTag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @param callback A callback function to invoke with the layer versions.
   * response.
   *
   * @return The cancellation token.
   */
  static client::CancellationToken GetLayerVersions(
      const client::OlpClient& client, int64_t version,
      boost::optional<std::string> billingTag,
      const LayerVersionsCallback& callback);

  /**
   * @brief Call to asynchronously retrieve metadata for all partitions in a
   * specified layer.
   * @param client Instance of OlpClient used to make REST request.
   * @param layerId Layer id.
   * @param version Specify the version for a versioned layer. Doesn't apply for
   * other layer types.
   * @param additionalFields Additional fields - dataSize, checksum,
   * compressedDataSize.
   * @param range Use this parameter to resume download of a large response for
   * versioned layers when there is a connection issue between the client and
   * server. Specify a single byte range offset like this: Range: bytes=10-.
   * This parameter is compliant with RFC 7233, but note that this parameter
   * only supports a single byte range. The range parameter can also be
   * specified as a query parameter, i.e. range=bytes=10-. For volatile layers
   * use the pagination links returned in the response body.
   * @param billingTag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @param callback A callback function to invoke with the partitions
   * response.
   *
   * @return The cancellation token.
   */
  static client::CancellationToken GetPartitions(
      const client::OlpClient& client, const std::string& layerId,
      boost::optional<int64_t> version,
      boost::optional<std::vector<std::string>> additionalFields,
      boost::optional<std::string> range,
      boost::optional<std::string> billingTag,
      const PartitionsCallback& callback);

  /**
   * @brief Call to asynchronously retrieve the latest metadata version for the
   * catalog.
   * @param client Instance of OlpClient used to make REST request.
   * @param startVersion The catalog version returned from a prior request. Save
   * the version from each request so it can used in the startVersion parameter
   * of subsequent requests. If the version from a prior request is not
   * avaliable, set the parameter to -1.
   * @param billingTag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @param callback A callback function to invoke with the catalog version.
   * response.
   *
   * @return The cancellation token.
   */
  static client::CancellationToken GetLatestCatalogVersion(
      const client::OlpClient& client, int64_t startVersion,
      boost::optional<std::string> billingTag,
      const CatalogVersionCallback& callback);
};

}  // namespace write

}  // namespace dataservice
}  // namespace olp
