/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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
#include "ExtendedApiResponse.h"
#include "QuadTreeIndex.h"
#include "generated/api/QueryApi.h"
#include "generated/model/Index.h"
#include "olp/dataservice/read/DataRequest.h"
#include "olp/dataservice/read/PartitionsRequest.h"
#include "olp/dataservice/read/Types.h"

#include "AsyncJsonStream.h"
#include "NamedMutex.h"
#include "PartitionsCacheRepository.h"

namespace olp {
namespace dataservice {
namespace read {

class TileRequest;

namespace repository {

/// The partition metadata response type.
using PartitionResponse = Response<model::Partition, client::NetworkStatistics>;
using QuadTreeIndexResponse =
    Response<QuadTreeIndex, client::NetworkStatistics>;

class PartitionsRepository {
 public:
  PartitionsRepository(client::HRN catalog, std::string layer,
                       client::OlpClientSettings settings,
                       const client::ApiLookupClient& client,
                       NamedMutexStorage storage = NamedMutexStorage());

  PartitionsResponse GetVolatilePartitions(
      const read::PartitionsRequest& request,
      const client::CancellationContext& context);

  QueryApi::PartitionsExtendedResponse GetVersionedPartitionsExtendedResponse(
      const read::PartitionsRequest& request, std::int64_t version,
      client::CancellationContext context, bool fail_on_cache_error = false);

  PartitionsResponse GetPartitionById(const DataRequest& request,
                                      porting::optional<int64_t> version,
                                      client::CancellationContext context);

  static model::Partition PartitionFromSubQuad(const model::SubQuad& sub_quad,
                                               const std::string& partition);

  PartitionResponse GetAggregatedTile(
      TileRequest request, porting::optional<int64_t> version,
      const client::CancellationContext& context);

  PartitionResponse GetTile(const TileRequest& request,
                            porting::optional<int64_t> version,
                            client::CancellationContext context,
                            const std::vector<std::string>& required_fields);

  client::ApiNoResponse ParsePartitionsStream(
      const std::shared_ptr<AsyncJsonStream>& async_stream,
      const PartitionsStreamCallback& partition_callback,
      client::CancellationContext context);

  void StreamPartitions(const std::shared_ptr<AsyncJsonStream>& async_stream,
                        std::int64_t version,
                        const std::vector<std::string>& additional_fields,
                        porting::optional<std::string> billing_tag,
                        const client::CancellationContext& context);

 private:
  QuadTreeIndexResponse GetQuadTreeIndexForTile(
      const TileRequest& request, porting::optional<int64_t> version,
      client::CancellationContext context,
      const std::vector<std::string>& required_fields);

  QueryApi::PartitionsExtendedResponse GetPartitionsExtendedResponse(
      const read::PartitionsRequest& request,
      porting::optional<std::int64_t> version,
      client::CancellationContext context,
      porting::optional<time_t> expiry = porting::none,
      bool fail_on_cache_error = false);

  QueryApi::PartitionsExtendedResponse QueryPartitionsInBatches(
      const client::OlpClient& client,
      const PartitionsRequest::PartitionIds& partitions,
      porting::optional<std::int64_t> version,
      const PartitionsRequest::AdditionalFields& additional_fields,
      porting::optional<std::string> billing_tag,
      client::CancellationContext context);

  const client::HRN catalog_;
  const std::string layer_id_;
  client::OlpClientSettings settings_;
  client::ApiLookupClient lookup_client_;
  PartitionsCacheRepository cache_;
  NamedMutexStorage storage_;
};
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
