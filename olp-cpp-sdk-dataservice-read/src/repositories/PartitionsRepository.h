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

#include <olp/core/client/ApiLookupClient.h>
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
  PartitionsRepository(client::HRN catalog, client::OlpClientSettings settings,
                       client::ApiLookupClient client);

  PartitionsResponse GetVersionedPartitions(
      std::string layer, const read::PartitionsRequest& request,
      std::int64_t version, client::CancellationContext context);

  PartitionsResponse GetVolatilePartitions(
      std::string layer, const read::PartitionsRequest& request,
      client::CancellationContext context);

  PartitionsResponse GetPartitionById(const std::string& layer,
                                      boost::optional<int64_t> version,
                                      const DataRequest& request,
                                      client::CancellationContext context);

  model::Partition PartitionFromSubQuad(const model::SubQuad& sub_quad,
                                        const std::string& partition);

  PartitionResponse GetAggregatedTile(const std::string& layer,
                                      const TileRequest& request,
                                      boost::optional<int64_t> version,
                                      client::CancellationContext context);

  PartitionResponse GetTile(const std::string& layer,
                            const TileRequest& request,
                            boost::optional<int64_t> version,
                            client::CancellationContext context);

 private:
  QuadTreeIndexResponse GetQuadTreeIndexForTile(
      const std::string& layer, const TileRequest& request,
      boost::optional<int64_t> version, client::CancellationContext context);

  PartitionsResponse GetPartitions(
      std::string layer, const read::PartitionsRequest& request,
      boost::optional<std::int64_t> version,
      client::CancellationContext context,
      boost::optional<time_t> expiry = boost::none);

  client::HRN catalog_;
  client::OlpClientSettings settings_;
  client::ApiLookupClient lookup_client_;
};
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
