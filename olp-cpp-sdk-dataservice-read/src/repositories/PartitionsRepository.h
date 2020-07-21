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

#include <string>
#include <vector>

#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include "QuadTreeIndex.h"
#include "generated/model/Index.h"
#include "olp/dataservice/read/DataRequest.h"
#include "olp/dataservice/read/PartitionsRequest.h"
#include "olp/dataservice/read/Types.h"

namespace olp {
namespace dataservice {
namespace read {
class TileRequest;
namespace repository {
class ApiRepository;
class CatalogRepository;
class PartitionsCacheRepository;

/// The partition metadata response type.
using PartitionResponse = Response<model::Partition>;
using QuadTreeIndexResponse = Response<QuadTreeIndex>;

class PartitionsRepository {
 public:
  static PartitionsResponse GetVersionedPartitions(
      client::HRN catalog, std::string layer,
      client::CancellationContext cancellation_context,
      read::PartitionsRequest data_request, client::OlpClientSettings settings);

  static PartitionsResponse GetVolatilePartitions(
      client::HRN catalog, std::string layer,
      client::CancellationContext cancellation_context,
      read::PartitionsRequest data_request, client::OlpClientSettings settings);

  static PartitionsResponse GetPartitionById(
      const client::HRN& catalog, const std::string& layer,
      boost::optional<int64_t> version,
      client::CancellationContext cancellation_context,
      const DataRequest& data_request, client::OlpClientSettings settings);

  static model::Partition PartitionFromSubQuad(const model::SubQuad& sub_quad,
                                               const std::string& partition);

  static PartitionResponse GetAggregatedTile(
      const client::HRN& catalog, const std::string& layer,
      client::CancellationContext cancellation_context,
      const TileRequest& request, boost::optional<int64_t> version,
      const client::OlpClientSettings& settings);

  static PartitionResponse GetTile(
      const client::HRN& catalog, const std::string& layer,
      client::CancellationContext cancellation_context,
      const TileRequest& request, boost::optional<int64_t> version,
      const client::OlpClientSettings& settings);

  static bool FindQuadTree(const client::HRN& catalog,
                           const client::OlpClientSettings& settings,
                           const std::string& layer,
                           boost::optional<int64_t> version,
                           const olp::geo::TileKey& tile_key,
                           read::QuadTreeIndex& tree);

 private:
  static QuadTreeIndexResponse GetQuadTreeIndexForTile(
      const client::HRN& catalog, const std::string& layer,
      client::CancellationContext cancellation_context,
      const TileRequest& request, boost::optional<int64_t> version,
      const client::OlpClientSettings& settings);

  static PartitionsResponse GetPartitions(
      client::HRN catalog, std::string layer,
      client::CancellationContext cancellation_context,
      read::PartitionsRequest request,
      const client::OlpClientSettings& settings,
      boost::optional<time_t> expiry = boost::none);
};
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
