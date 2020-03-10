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

#include "PartitionsRepository.h"

#include <olp/core/client/Condition.h>
#include <olp/core/logging/Log.h>

#include "ApiClientLookup.h"
#include "CatalogRepository.h"
#include "PartitionsCacheRepository.h"
#include "generated/api/MetadataApi.h"
#include "generated/api/QueryApi.h"
#include "olp/dataservice/read/CatalogRequest.h"
#include "olp/dataservice/read/CatalogVersionRequest.h"
#include "olp/dataservice/read/DataRequest.h"
#include "olp/dataservice/read/PartitionsRequest.h"
#include "olp/dataservice/read/TileRequest.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {
using namespace olp::client;

namespace {
constexpr auto kLogTag = "PartitionsRepository";
constexpr uint32_t kMaxQuadTreeIndexDepth = 4u;

using LayerVersionReponse = ApiResponse<int64_t, client::ApiError>;
using LayerVersionCallback = std::function<void(LayerVersionReponse)>;

ApiResponse<boost::optional<time_t>, ApiError> TtlForLayer(
    const std::vector<model::Layer>& layers, const std::string& layer_id) {
  for (const auto& layer : layers) {
    if (layer.GetId() == layer_id) {
      boost::optional<time_t> expiry;
      if (layer.GetTtl()) {
        expiry = *layer.GetTtl() / 1000;
      }
      return expiry;
    }
  }

  return ApiError(client::ErrorCode::NotFound,
                  "Layer specified doesn't exist.");
}

}  // namespace

PartitionsResponse PartitionsRepository::GetVersionedPartitions(
    client::HRN catalog, std::string layer,
    client::CancellationContext cancellation_context, PartitionsRequest request,
    client::OlpClientSettings settings) {
  if (!request.GetVersion()) {
    // get latest version of the layer if it wasn't set by the user
    auto latest_version_response =
        repository::CatalogRepository::GetLatestVersion(
            catalog, cancellation_context,
            CatalogVersionRequest()
                .WithFetchOption(request.GetFetchOption())
                .WithBillingTag(request.GetBillingTag()),
            settings);
    if (!latest_version_response.IsSuccessful()) {
      return latest_version_response.GetError();
    }
    request.WithVersion(latest_version_response.GetResult().GetVersion());
  }
  return GetPartitions(std::move(catalog), std::move(layer),
                       std::move(cancellation_context), std::move(request),
                       std::move(settings));
}

PartitionsResponse PartitionsRepository::GetVolatilePartitions(
    client::HRN catalog, std::string layer,
    client::CancellationContext cancellation_context, PartitionsRequest request,
    client::OlpClientSettings settings) {
  request.WithVersion(boost::none);

  CatalogRequest catalog_request;
  catalog_request.WithBillingTag(request.GetBillingTag())
      .WithFetchOption(request.GetFetchOption());

  auto catalog_response = repository::CatalogRepository::GetCatalog(
      catalog, cancellation_context, catalog_request, settings);

  if (!catalog_response.IsSuccessful()) {
    return catalog_response.GetError();
  }

  auto expiry_response =
      TtlForLayer(catalog_response.GetResult().GetLayers(), layer);

  if (!expiry_response.IsSuccessful()) {
    return expiry_response.GetError();
  }

  return GetPartitions(std::move(catalog), std::move(layer),
                       cancellation_context, std::move(request),
                       std::move(settings), expiry_response.MoveResult());
}

PartitionsResponse PartitionsRepository::GetPartitions(
    client::HRN catalog, std::string layer,
    client::CancellationContext cancellation_context, PartitionsRequest request,
    const client::OlpClientSettings& settings, boost::optional<time_t> expiry) {
  auto fetch_option = request.GetFetchOption();

  repository::PartitionsCacheRepository repository(catalog, settings.cache);

  if (fetch_option != OnlineOnly) {
    auto cached_partitions = repository.Get(request, layer);
    if (cached_partitions) {
      OLP_SDK_LOG_INFO_F(kLogTag, "cache data '%s' found!",
                         request.CreateKey(layer).c_str());
      return cached_partitions.get();
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' not found!",
                         request.CreateKey(layer).c_str());
      return ApiError(ErrorCode::NotFound,
                      "Cache only resource not found in cache (partition).");
    }
  }

  PartitionsResponse response;

  const auto& partition_ids = request.GetPartitionIds();
  
  if (partition_ids.empty()) {
    auto metadata_api =
        ApiClientLookup::LookupApi(catalog, cancellation_context, "metadata",
                                   "v1", fetch_option, std::move(settings));

    if (!metadata_api.IsSuccessful()) {
      return metadata_api.GetError();
    }

    response = MetadataApi::GetPartitions(
        metadata_api.GetResult(), layer, request.GetVersion(),
        request.GetAdditionalFields(), boost::none, request.GetBillingTag(),
        cancellation_context);
  } else {
    auto query_api =
        ApiClientLookup::LookupApi(catalog, cancellation_context, "query", "v1",
                                   fetch_option, std::move(settings));

    if (!query_api.IsSuccessful()) {
      return query_api.GetError();
    }

    response = QueryApi::GetPartitionsbyId(
        query_api.GetResult(), layer, partition_ids, request.GetVersion(),
        request.GetAdditionalFields(), request.GetBillingTag(),
        cancellation_context);
  }

  // Save all partitions only when downloaded via metadata API
  const bool is_layer_metadata = partition_ids.empty();

  if (response.IsSuccessful() && fetch_option != OnlineOnly) {
    OLP_SDK_LOG_INFO_F(kLogTag, "put '%s' to cache",
                       request.CreateKey(layer).c_str());
    repository.Put(request, response.GetResult(), layer, expiry,
                   is_layer_metadata);
  }
  if (!response.IsSuccessful()) {
    const auto& error = response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_INFO_F(kLogTag, "clear '%s' cache",
                         request.CreateKey(layer).c_str());
      repository.Clear(layer);
    }
  }

  return response;
}

PartitionsResponse PartitionsRepository::GetPartitionById(
    const client::HRN& catalog, const std::string& layer,
    client::CancellationContext cancellation_context,
    const DataRequest& data_request, client::OlpClientSettings settings) {
  const auto& partition_id = data_request.GetPartitionId();
  if (!partition_id) {
    return ApiError(ErrorCode::PreconditionFailed, "Partition Id is missing");
  }

  const auto& version = data_request.GetVersion();

  std::chrono::seconds timeout{settings.retry_settings.timeout};
  repository::PartitionsCacheRepository repository(catalog, settings.cache);

  const std::vector<std::string> partitions{partition_id.value()};
  PartitionsRequest partition_request;
  partition_request.WithBillingTag(data_request.GetBillingTag())
      .WithVersion(version);

  auto fetch_option = data_request.GetFetchOption();

  if (fetch_option != OnlineOnly) {
    auto cached_partitions =
        repository.Get(partition_request, partitions, layer);
    if (cached_partitions.GetPartitions().size() == partitions.size()) {
      OLP_SDK_LOG_INFO_F(kLogTag, "cache data '%s' found!",
                         data_request.CreateKey(layer).c_str());
      return cached_partitions;
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' not found!",
                         data_request.CreateKey(layer).c_str());
      return ApiError(ErrorCode::NotFound,
                      "Cache only resource not found in cache (partition).");
    }
  }

  auto query_api =
      ApiClientLookup::LookupApi(catalog, cancellation_context, "query", "v1",
                                 fetch_option, std::move(settings));

  if (!query_api.IsSuccessful()) {
    return query_api.GetError();
  }

  const client::OlpClient& client = query_api.GetResult();

  PartitionsResponse query_response = QueryApi::GetPartitionsbyId(
      client, layer, partitions, version, {}, data_request.GetBillingTag(),
      cancellation_context);

  if (query_response.IsSuccessful() && fetch_option != OnlineOnly) {
    OLP_SDK_LOG_INFO_F(kLogTag, "put '%s' to cache",
                       data_request.CreateKey(layer).c_str());
    repository.Put(partition_request, query_response.GetResult(), layer,
                   boost::none /* TODO: expiration */);
  }
  if (!query_response.IsSuccessful()) {
    const auto& error = query_response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_INFO_F(kLogTag, "clear '%s' cache",
                         data_request.CreateKey(layer).c_str());
      // Delete partitions only but not the layer
      repository.ClearPartitions(partition_request, partitions, layer);
    }
  }

  return query_response;
}

model::Partition PartitionsRepository::PartitionFromSubQuad(
    const model::SubQuad& sub_quad, const std::string& partition) {
  model::Partition ret;
  ret.SetPartition(partition);
  ret.SetDataHandle(sub_quad.GetDataHandle());
  ret.SetVersion(sub_quad.GetVersion());
  ret.SetDataSize(sub_quad.GetDataSize());
  ret.SetChecksum(sub_quad.GetChecksum());
  ret.SetCompressedDataSize(sub_quad.GetCompressedDataSize());
  return ret;
}

PartitionsResponse PartitionsRepository::GetPartitionForVersionedTile(
    const client::HRN& catalog, const std::string& layer_id,
    const TileRequest& request, int64_t version,
    client::CancellationContext context,
    const client::OlpClientSettings& settings) {
  auto tile = request.GetTileKey().ToHereTile();
  auto cached_partitions =
      GetTileFromCache(catalog, layer_id, request, version, settings);
  if (!cached_partitions.GetPartitions().empty()) {
    OLP_SDK_LOG_INFO_F(kLogTag, "cache data '%s' found!",
                       request.CreateKey(layer_id).c_str());

    for (const auto& partition : cached_partitions.GetPartitions()) {
      // find data handle for requested tile
      if (partition.GetPartition().compare(tile) == 0) {
        OLP_SDK_LOG_INFO_F(kLogTag, "Requested tile data handle: %s.",
                           partition.GetDataHandle().c_str());
        model::Partitions result;
        result.SetPartitions({partition});
        return std::move(result);
      }
    }
  } else if (request.GetFetchOption() == CacheOnly) {
    OLP_SDK_LOG_INFO_F(kLogTag, "cache tile '%s' not found!",
                       request.CreateKey(layer_id).c_str());
    return ApiError(ErrorCode::NotFound,
                    "Cache only resource not found in cache (data).");
  }

  return QueryPartitionForVersionedTile(catalog, layer_id, request, version,
                                        context, settings);
}

PartitionsResponse PartitionsRepository::QueryPartitionForVersionedTile(
    const client::HRN& catalog, const std::string& layer_id,
    const TileRequest& request, int64_t version,
    client::CancellationContext context, client::OlpClientSettings settings) {
  auto fetch_option = request.GetFetchOption();
  const auto& tile_key = request.GetTileKey();
  auto tile = tile_key.ToHereTile();
  auto query_api = ApiClientLookup::LookupApi(
      catalog, context, "query", "v1", request.GetFetchOption(), settings);

  if (!query_api.IsSuccessful()) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "QueryPartitionForVersionedTile: LookupApi failed.");
    return query_api.GetError();
  }

  auto quad_tree = QueryApi::QuadTreeIndex(
      query_api.GetResult(), layer_id, version, tile, kMaxQuadTreeIndexDepth,
      boost::none, request.GetBillingTag(), context);

  if (!quad_tree.IsSuccessful()) {
    OLP_SDK_LOG_ERROR_F(kLogTag,
                        "QuadTreeIndex failed (%s, %" PRId64 ", %" PRId32 ")",
                        tile.c_str(), version, kMaxQuadTreeIndexDepth);
    return quad_tree.GetError();
  }
  model::Partitions result;
  model::Partitions partitions;
  auto& partitions_vector = partitions.GetMutablePartitions();
  const auto& subquads = quad_tree.GetResult().GetSubQuads();
  partitions_vector.reserve(subquads.size());

  OLP_SDK_LOG_DEBUG_F(kLogTag, "Requested tile subquads size %lu.",
                      subquads.size());

  for (const auto& subquad : subquads) {
    auto subtile =
        request.GetTileKey().AddedSubHereTile(subquad->GetSubQuadKey());
    // add partitions for caching
    partitions_vector.emplace_back(
        PartitionFromSubQuad(*subquad, subtile.ToHereTile()));
    // find data handle for requested tile
    if (subtile == tile_key) {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "Requested tile data handle: %s.",
          partitions.GetMutablePartitions().back().GetDataHandle().c_str());
      result.SetPartitions({partitions.GetMutablePartitions().back()});
    }
  }

  if (fetch_option != OnlineOnly) {
    // add partitions to cache
    repository::PartitionsCacheRepository repository(catalog, settings.cache);
    repository.Put(PartitionsRequest().WithVersion(version), partitions,
                   layer_id, boost::none);
  }

  if (result.GetPartitions().size() == 0) {
    return ApiError(ErrorCode::NotFound,
                    "Partition for requested TileKey was not found.");
  }
  return std::move(result);
}

model::Partitions PartitionsRepository::GetTileFromCache(
    const client::HRN& catalog, const std::string& layer_id,
    const TileRequest& request, int64_t version,
    const client::OlpClientSettings& settings) {
  if (request.GetFetchOption() != OnlineOnly) {
    repository::PartitionsCacheRepository repository(catalog, settings.cache);

    auto partition_request = PartitionsRequest()
                                 .WithBillingTag(request.GetBillingTag())
                                 .WithVersion(version);

    const std::vector<std::string> partitions{
        request.GetTileKey().ToHereTile()};
    return repository.Get(partition_request, partitions, layer_id);
  }
  return {};
}
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
