/*
 * Copyright (C) 2019-2022 HERE Europe B.V.
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
#include "ExtendedApiResponse.h"
#include "generated/model/LayerVersions.h"
#include "olp/dataservice/read/model/Partitions.h"
#include "olp/dataservice/read/model/VersionInfos.h"
#include "olp/dataservice/read/model/VersionResponse.h"
#include "olp/dataservice/read/model/VersionsResponse.h"

namespace olp {
namespace client {
class OlpClient;
class CancellationContext;
}  // namespace client

namespace dataservice {
namespace read {
/**
 * @brief Api to get information about catalogs, layers, and partitions.
 */
class MetadataApi {
 public:
  using VersionsResponse =
      client::ApiResponse<model::VersionInfos, client::ApiError>;

  using CatalogVersionResponse =
      client::ApiResponse<model::VersionResponse, client::ApiError>;

  using LayerVersionsResponse =
      client::ApiResponse<model::LayerVersions, client::ApiError>;

  using PartitionsExtendedResponse =
      ExtendedApiResponse<model::Partitions, client::ApiError,
                          client::NetworkStatistics>;

  using CatalogVersions = std::vector<model::CatalogVersion>;

  using CompatibleVersionsResponse =
      client::ApiResponse<model::VersionsResponse, client::ApiError>;

  /**
   * @brief Retrieves the latest metadata version for each layer of a specified
   * catalog metadata version.
   * @param client Instance of OlpClient used to make REST request.
   * @param version The catalog version.
   * @param billing_tag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @param context A CancellationContext, which can be used to cancel request.
   *
   * @return The LayerVersions response.
   */
  static LayerVersionsResponse GetLayerVersions(
      const client::OlpClient& client, int64_t version,
      boost::optional<std::string> billing_tag,
      const client::CancellationContext& context);

  /**
   * @brief Retrieves metadata for all partitions in a specified layer.
   * @param client Instance of OlpClient used to make REST request.
   * @param layerId Layer id.
   * @param version Specify the version for a versioned layer. Doesn't apply for
   * other layer types.
   * @param additional_fields Additional fields - dataSize, checksum,
   * compressedDataSize.
   * @param range Use this parameter to resume download of a large response for
   * versioned layers when there is a connection issue between the client and
   * server. Specify a single byte range offset like this: Range: bytes=10-.
   * This parameter is compliant with RFC 7233, but note that this parameter
   * only supports a single byte range. The range parameter can also be
   * specified as a query parameter, i.e. range=bytes=10-. For volatile layers
   * use the pagination links returned in the response body.
   * @param billing_tag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @param context A CancellationContext, which can be used to cancel request.
   *
   * @return  The result of this operation as an extended client::ApiResponse
   * object with \c model::Partitions as a result.
   */
  static PartitionsExtendedResponse GetPartitions(
      const client::OlpClient& client, const std::string& layerId,
      boost::optional<int64_t> version,
      const std::vector<std::string>& additional_fields,
      boost::optional<std::string> range,
      boost::optional<std::string> billing_tag,
      const client::CancellationContext& context);

  /**
   * @brief Retrieves the latest metadata version for the catalog.
   * @param client Instance of OlpClient used to make REST request.
   * @param start_version The catalog version returned from a prior request.
   * Save the version from each request so it can used in the startVersion
   * parameter of subsequent requests. If the version from a prior request is
   * not avaliable, set the parameter to -1.
   * @param billing_tag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @param context A CancellationContext, which can be used to cancel request.
   *
   * @return The CatalogVersion response.
   */
  static CatalogVersionResponse GetLatestCatalogVersion(
      const client::OlpClient& client, int64_t start_version,
      boost::optional<std::string> billing_tag,
      const client::CancellationContext& context);

  static VersionsResponse ListVersions(
      const client::OlpClient& client, int64_t start_version,
      int64_t end_version, boost::optional<std::string> billing_tag,
      const client::CancellationContext& context);

  static CompatibleVersionsResponse GetCompatibleVersions(
      const client::OlpClient& client, const CatalogVersions& dependencies,
      int32_t limit, const client::CancellationContext& context);
};

}  // namespace read

}  // namespace dataservice
}  // namespace olp
