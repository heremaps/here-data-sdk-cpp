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
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/HttpResponse.h>
#include <olp/dataservice/read/ExtendedApiResponse.h>
#include <boost/optional.hpp>
#include "generated/model/Index.h"
#include "olp/dataservice/read/model/Partitions.h"

namespace olp {
namespace client {
class OlpClient;
}

namespace dataservice {
namespace read {
/**
 * @brief Api to get metadata for catalogs and partitions.
 */
class QueryApi {
 public:
  using QuadTreeIndexResponse =
      client::ApiResponse<model::Index, client::ApiError>;

  using PartitionsExtendedResponse =
      ExtendedApiResponse<model::Partitions, client::ApiError,
                          client::NetworkStatistics>;

  /**
   * @brief Call to synchronously retrieve metadata for specified partitions in
   * a specified layer.
   * @param client Instance of OlpClient used to make REST request.
   * @param layer_id Layer id.
   * @param partition Partition ids to use for filtering. You can specify
   * multiple partitions by using this parameter multiple times. Maximum allowed
   * partitions ids per call is 100.
   * @param version Specify the version for a versioned layer. Doesn't apply for
   * other layer types.
   * @param additional_fields Additional fields - dataSize, checksum,
   * compressedDataSize.
   * @param billing_tag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @param callback A callback function to invoke with the partitions
   * response.
   * @param context A CancellationContext instance which can be used to cancel
   * call of this method.
   * @return  The result of this operation as an extended client::ApiResponse
   * object with \c model::Partitions as a result.
   */
  static PartitionsExtendedResponse GetPartitionsbyId(
      const client::OlpClient& client, const std::string& layer_id,
      const std::vector<std::string>& partitions,
      boost::optional<int64_t> version,
      const std::vector<std::string>& additional_fields,
      boost::optional<std::string> billing_tag,
      client::CancellationContext context);

  /**
   * @brief Gets index metadata
   * Gets metadata synchronously for the requested index. Only available for
   * layers where the partitioning scheme is &#x60;heretile&#x60;.
   *
   * @param layer_id The ID of the layer specified in the request.
   * Content of this parameter must refer to a valid layer already configured
   * in the catalog configuration. Exactly one layer ID must be
   * provided.
   * @param quad_key The geometric area specified by an index in the request,
   * represented as a HERE tile
   * @param version The version of the catalog against
   * which to run the query. Must be a valid catalog version.
   * @param depth The recursion depth
   * of the response. If set to 0, the response includes only data for the
   * quad_key specified in the request. In this way, depth describes the maximum
   * length of the subQuadKeys in the response. The maximum allowed value for
   * the depth parameter is 4.
   * @param additional_fields Additional fields - &#x60;dataSize&#x60;,
   * &#x60;checksum&#x60;, &#x60;compressedDataSize&#x60;
   * @param billing_tag Billing Tag is an optional free-form tag used to
   * group billing records together. If supplied, it must be between 4 - 16
   * characters and  contain only alphanumeric ASCII characters  [A-Za-z0-9].
   * Grouping billing records by billing tag will be available in a future
   * release.
   * @param context A CancellationContext instance which can be used to cancel
   * call of this method.
   * @return HttpResponse of this operation.
   **/
  static olp::client::HttpResponse QuadTreeIndex(
      const client::OlpClient& client, const std::string& layer_id,
      const std::string& quad_key, boost::optional<int64_t> version,
      int32_t depth,
      boost::optional<std::vector<std::string>> additional_fields,
      boost::optional<std::string> billing_tag,
      client::CancellationContext context);

  /**
   * @brief Gets index metadata
   * Gets metadata synchronously for the requested index. Only available for
   * layers where the partitioning scheme is &#x60;heretile&#x60;.
   *
   * @param layer_id The ID of the layer specified in the request.
   * Content of this parameter must refer to a valid layer already configured
   * in the catalog configuration. Exactly one layer ID must be
   * provided.
   * @param quad_key The geometric area specified by an index in the request,
   * represented as a HERE tile
   * @param depth The recursion depth
   * of the response. If set to 0, the response includes only data for the
   * quad_key specified in the request. In this way, depth describes the maximum
   * length of the subQuadKeys in the response. The maximum allowed value for
   * the depth parameter is 4.
   * @param additional_fields Additional fields - &#x60;dataSize&#x60;,
   * &#x60;checksum&#x60;, &#x60;compressedDataSize&#x60;
   * @param billing_tag Billing Tag is an optional free-form tag used to
   * group billing records together. If supplied, it must be between 4 - 16
   * characters and  contain only alphanumeric ASCII characters  [A-Za-z0-9].
   * Grouping billing records by billing tag will be available in a future
   * release.
   * @param context A CancellationContext instance which can be used to cancel
   * call of this method.
   * @param The result of this operation as a client::ApiResponse object with \c
   * model::Index as a result
   **/
  static QuadTreeIndexResponse QuadTreeIndexVolatile(
      const client::OlpClient& client, const std::string& layer_id,
      const std::string& quad_key, int32_t depth,
      boost::optional<std::vector<std::string>> additional_fields,
      boost::optional<std::string> billing_tag,
      client::CancellationContext context);
};

}  // namespace read

}  // namespace dataservice
}  // namespace olp
