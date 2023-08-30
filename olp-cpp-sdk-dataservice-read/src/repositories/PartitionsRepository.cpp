/*
 * Copyright (C) 2019-2023 HERE Europe B.V.
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

#include <algorithm>
#include <utility>

#include <olp/core/cache/KeyGenerator.h>
#include <olp/core/client/Condition.h>
#include <olp/core/logging/Log.h>
#include <boost/functional/hash.hpp>
#include "CatalogRepository.h"
#include "generated/api/MetadataApi.h"
#include "generated/api/QueryApi.h"
#include "olp/dataservice/read/CatalogRequest.h"
#include "olp/dataservice/read/CatalogVersionRequest.h"
#include "olp/dataservice/read/DataRequest.h"
#include "olp/dataservice/read/PartitionsRequest.h"
#include "olp/dataservice/read/TileRequest.h"

namespace {
namespace client = olp::client;
namespace read = olp::dataservice::read;
namespace model = olp::dataservice::read::model;
namespace repository = olp::dataservice::read::repository;

constexpr auto kLogTag = "PartitionsRepository";
constexpr auto kAggregateQuadTreeDepth = 4;
constexpr auto kQueryRequestLimit = 100;

using LayerVersionReponse = client::ApiResponse<int64_t, client::ApiError>;
using LayerVersionCallback = std::function<void(LayerVersionReponse)>;

client::ApiResponse<boost::optional<time_t>, client::ApiError> TtlForLayer(
    const std::vector<model::Layer>& layers, const std::string& layer_id) {
  auto layer_it = std::find_if(
      std::begin(layers), std::end(layers),
      [&](const model::Layer& layer) { return layer.GetId() == layer_id; });

  if (layer_it == std::end(layers)) {
    return client::ApiError::NotFound("Layer specified doesn't exist");
  }

  auto ttl = layer_it->GetTtl();

  return ttl ? boost::make_optional<time_t>(ttl.value() / 1000) : boost::none;
}

boost::optional<model::Partition> FindPartition(
    const read::QuadTreeIndex& quad_tree, const read::TileRequest& request,
    bool aggregated) {
  const auto& tile_key = request.GetTileKey();

  // Look for requested tile in the quad tree or its closest ancestor
  const auto found_index_data = quad_tree.Find(tile_key, aggregated);

  if (!found_index_data) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "FindPartition: tile not found, tile='%s', depth='%" PRId32
        "', aggregated='%s'",
        tile_key.ToHereTile().c_str(), kAggregateQuadTreeDepth,
        aggregated ? "true" : "false");
    return boost::none;
  }

  const auto& index_data = found_index_data.get();

  model::Partition partition;
  partition.SetDataHandle(index_data.data_handle);
  partition.SetPartition(index_data.tile_key.ToHereTile());
  if (!index_data.checksum.empty()) {
    partition.SetChecksum(index_data.checksum);
  }
  if (!index_data.crc.empty()) {
    partition.SetCrc(index_data.crc);
  }
  if (index_data.data_size != -1) {
    partition.SetDataSize(index_data.data_size);
  }
  if (index_data.compressed_data_size != -1) {
    partition.SetCompressedDataSize(index_data.compressed_data_size);
  }

  return partition;
}

std::string HashPartitions(
    const read::PartitionsRequest::PartitionIds& partitions) {
  size_t seed = 0;
  for (const auto& partition : partitions) {
    boost::hash_combine(seed, partition);
  }
  return std::to_string(seed);
}

/// Check if all the requested additional fields are cached
bool CheckAdditionalFields(
    const boost::optional<std::vector<std::string>>& additional_fields,
    const read::QuadTreeIndex& cached_tree) {
  if (!additional_fields) {
    return true;
  }

  auto find_field = [&](const std::string& field) -> bool {
    return std::find(additional_fields->cbegin(), additional_fields->cend(),
                     field) != additional_fields->cend();
  };

  auto checksum_requested = find_field(read::PartitionsRequest::kChecksum);
  auto crc_requested = find_field(read::PartitionsRequest::kCrc);
  auto data_size_requested = find_field(read::PartitionsRequest::kDataSize);
  auto compressed_data_size_requested =
      find_field(read::PartitionsRequest::kCompressedDataSize);

  for (const auto& index_data : cached_tree.GetIndexData()) {
    if (checksum_requested && index_data.checksum.empty()) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "Additional field 'checksum' is not found in "
                            "index data, tile='%s'",
                            index_data.tile_key.ToHereTile().c_str());
      return false;
    }

    if (crc_requested && index_data.crc.empty()) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "Additional field 'crc' is not found in "
                            "index data, tile='%s'",
                            index_data.tile_key.ToHereTile().c_str());
      return false;
    }

    if (data_size_requested && index_data.data_size == -1) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "Additional field 'data_size' is not found in "
                            "index data, tile='%s'",
                            index_data.tile_key.ToHereTile().c_str());
      return false;
    }

    if (compressed_data_size_requested &&
        index_data.compressed_data_size == -1) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag,
          "Additional field 'compressed_data_size' is not found in "
          "index data, tile='%s'",
          index_data.tile_key.ToHereTile().c_str());
      return false;
    }
  }

  return true;
}
}  // namespace

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

PartitionsRepository::PartitionsRepository(client::HRN catalog,
                                           std::string layer,
                                           client::OlpClientSettings settings,
                                           client::ApiLookupClient client,
                                           NamedMutexStorage storage)
    : catalog_(std::move(catalog)),
      layer_id_(std::move(layer)),
      settings_(std::move(settings)),
      lookup_client_(std::move(client)),
      cache_(catalog_, layer_id_, settings_.cache,
             settings_.default_cache_expiration),
      storage_(std::move(storage)) {}

QueryApi::PartitionsExtendedResponse
PartitionsRepository::GetVersionedPartitionsExtendedResponse(
    const read::PartitionsRequest& request, std::int64_t version,
    client::CancellationContext context, const bool fail_on_cache_error) {
  return GetPartitionsExtendedResponse(request, version, std::move(context),
                                       boost::none, fail_on_cache_error);
}

PartitionsResponse PartitionsRepository::GetVolatilePartitions(
    const PartitionsRequest& request, client::CancellationContext context) {
  auto catalog_request = CatalogRequest()
                             .WithBillingTag(request.GetBillingTag())
                             .WithFetchOption(request.GetFetchOption());

  CatalogRepository repository(catalog_, settings_, lookup_client_);
  auto catalog_response = repository.GetCatalog(catalog_request, context);

  if (!catalog_response.IsSuccessful()) {
    return catalog_response.GetError();
  }

  auto expiry_response =
      TtlForLayer(catalog_response.GetResult().GetLayers(), layer_id_);
  if (!expiry_response.IsSuccessful()) {
    return expiry_response.GetError();
  }

  return GetPartitionsExtendedResponse(request, boost::none, context,
                                       expiry_response.MoveResult());
}

QueryApi::PartitionsExtendedResponse
PartitionsRepository::GetPartitionsExtendedResponse(
    const PartitionsRequest& request, boost::optional<std::int64_t> version,
    client::CancellationContext context, boost::optional<time_t> expiry,
    const bool fail_on_cache_error) {
  auto fetch_option = request.GetFetchOption();
  const auto key = request.CreateKey(layer_id_);

  const auto catalog_str = catalog_.ToCatalogHRNString();

  const auto& partition_ids = request.GetPartitionIds();

  // Temporary workaround for merging the same requests. Should be removed after
  // OlpClient could handle that.
  const auto detail =
      partition_ids.empty() ? "" : HashPartitions(partition_ids);
  const auto version_str = version ? std::to_string(*version) : "";

  NamedMutex mutex(storage_, catalog_str + layer_id_ + version_str + detail,
                   context);
  std::unique_lock<NamedMutex> lock(mutex, std::defer_lock);

  // If we are not planning to go online or access the cache, do not lock.
  if (fetch_option != CacheOnly && fetch_option != OnlineOnly) {
    lock.lock();
  }

  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    auto cached_partitions = cache_.Get(request, version);
    if (cached_partitions) {
      OLP_SDK_LOG_DEBUG_F(kLogTag,
                          "GetPartitions found in cache, hrn='%s', key='%s'",
                          catalog_str.c_str(), key.c_str());
      return cached_partitions.get();
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_DEBUG_F(
          kLogTag, "GetPartitions not found in cache, hrn='%s', key='%s'",
          catalog_str.c_str(), key.c_str());
      return client::ApiError::NotFound(
          "CacheOnly: resource not found in cache");
    }
  }

  QueryApi::PartitionsExtendedResponse response;

  if (partition_ids.empty()) {
    auto metadata_api = lookup_client_.LookupApi(
        "metadata", "v1", static_cast<client::FetchOptions>(fetch_option),
        context);

    if (!metadata_api.IsSuccessful()) {
      return metadata_api.GetError();
    }

    response =
        MetadataApi::GetPartitions(metadata_api.GetResult(), layer_id_, version,
                                   request.GetAdditionalFields(), boost::none,
                                   request.GetBillingTag(), context);
  } else {
    auto query_api = lookup_client_.LookupApi(
        "query", "v1", static_cast<client::FetchOptions>(fetch_option),
        context);

    if (!query_api.IsSuccessful()) {
      return query_api.GetError();
    }

    if (partition_ids.size() <= kQueryRequestLimit) {
      response = QueryApi::GetPartitionsbyId(
          query_api.GetResult(), layer_id_, partition_ids, version,
          request.GetAdditionalFields(), request.GetBillingTag(), context);
    } else {
      response = QueryPartitionsInBatches(
          query_api.GetResult(), partition_ids, version,
          request.GetAdditionalFields(), request.GetBillingTag(), context);
    }
  }
  // Save all partitions only when downloaded via metadata API
  const bool is_layer_metadata = partition_ids.empty();
  if (response.IsSuccessful() && fetch_option != OnlineOnly) {
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "GetPartitions put to cache, hrn='%s', key='%s'",
                        catalog_str.c_str(), key.c_str());
    const auto put_result =
        cache_.Put(response.GetResult(), version, expiry, is_layer_metadata);
    if (!put_result.IsSuccessful() && fail_on_cache_error) {
      OLP_SDK_LOG_ERROR_F(kLogTag,
                          "Failed to write data to cache, hrn='%s', key='%s'",
                          catalog_.ToCatalogHRNString().c_str(), key.c_str());
      return put_result.GetError();
    }
  }
  if (!response.IsSuccessful()) {
    const auto& error = response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag,
          "GetPartitions 403 received, remove from cache, hrn='%s', key='%s'",
          catalog_str.c_str(), key.c_str());
      cache_.Clear();
    }
  }

  return response;
}

PartitionsResponse PartitionsRepository::GetPartitionById(
    const DataRequest& request, boost::optional<int64_t> version,
    client::CancellationContext context) {
  const auto& partition_id = request.GetPartitionId();
  if (!partition_id) {
    return client::ApiError::PreconditionFailed("Partition Id is missing");
  }

  auto fetch_option = request.GetFetchOption();

  const auto request_key =
      catalog_.ToString() + request.CreateKey(layer_id_, version);

  NamedMutex mutex(storage_, request_key, context);
  std::unique_lock<repository::NamedMutex> lock(mutex, std::defer_lock);

  // If we are not planning to go online or access the cache, do not lock.
  if (fetch_option != CacheOnly && fetch_option != OnlineOnly) {
    lock.lock();
  }

  std::chrono::seconds timeout{settings_.retry_settings.timeout};
  const auto key = request.CreateKey(layer_id_, version);

  const std::vector<std::string> partitions{partition_id.value()};

  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    auto cached_partitions = cache_.Get(partitions, version);
    if (cached_partitions.GetPartitions().size() == partitions.size()) {
      OLP_SDK_LOG_DEBUG_F(kLogTag,
                          "GetPartitionById found in cache, hrn='%s', key='%s'",
                          catalog_.ToCatalogHRNString().c_str(), key.c_str());
      return cached_partitions;
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_DEBUG_F(
          kLogTag, "GetPartitionById not found in cache, hrn='%s', key='%s'",
          catalog_.ToCatalogHRNString().c_str(), key.c_str());
      return client::ApiError::NotFound(
          "CacheOnly: resource not found in cache");
    }
  }

  auto query_api = lookup_client_.LookupApi(
      "query", "v1", static_cast<client::FetchOptions>(fetch_option), context);

  if (!query_api.IsSuccessful()) {
    return query_api.GetError();
  }

  const client::OlpClient& client = query_api.GetResult();

  PartitionsResponse query_response =
      QueryApi::GetPartitionsbyId(client, layer_id_, partitions, version, {},
                                  request.GetBillingTag(), context);

  if (query_response.IsSuccessful() && fetch_option != OnlineOnly) {
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "GetPartitionById put to cache, hrn='%s', key='%s'",
                        catalog_.ToCatalogHRNString().c_str(), key.c_str());
    cache_.Put(query_response.GetResult(), version, boost::none);
  } else if (!query_response.IsSuccessful()) {
    const auto& error = query_response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "GetPartitionById 403 received, remove from cache, "
                            "hrn='%s', key='%s'",
                            catalog_.ToCatalogHRNString().c_str(), key.c_str());
      // Delete partitions only but not the layer
      cache_.ClearPartitions(partitions, version);
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

QuadTreeIndexResponse PartitionsRepository::GetQuadTreeIndexForTile(
    const TileRequest& request, boost::optional<int64_t> version,
    client::CancellationContext context,
    boost::optional<std::vector<std::string>> additional_fields) {
  auto fetch_option = request.GetFetchOption();
  const auto& tile_key = request.GetTileKey();

  const auto& root_tile_key = tile_key.ChangedLevelBy(-kAggregateQuadTreeDepth);
  const auto root_tile_here = root_tile_key.ToHereTile();

  const auto quad_cache_key = cache::KeyGenerator::CreateQuadTreeKey(
      catalog_.ToCatalogHRNString(), layer_id_, root_tile_key, version,
      kAggregateQuadTreeDepth);

  NamedMutex mutex(storage_, quad_cache_key, context);
  std::unique_lock<NamedMutex> lock(mutex, std::defer_lock);

  // If we are not planning to go online or access the cache, do not lock.
  if (fetch_option != CacheOnly && fetch_option != OnlineOnly) {
    lock.lock();
  }

  // Look for QuadTree covering the tile in the cache
  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    QuadTreeIndex cached_tree;
    if (cache_.FindQuadTree(tile_key, version, cached_tree)) {
      if (CheckAdditionalFields(additional_fields, cached_tree)) {
        OLP_SDK_LOG_DEBUG_F(kLogTag,
                            "GetQuadTreeIndexForTile found in cache, "
                            "tile='%s', depth='%" PRId32 "'",
                            tile_key.ToHereTile().c_str(),
                            kAggregateQuadTreeDepth);

        return QuadTreeIndexResponse(std::move(cached_tree));
      } else {
        OLP_SDK_LOG_WARNING_F(
            kLogTag,
            "GetQuadTreeIndexForTile found in cache, but not all "
            "the required additional fields are present in cache, "
            "tile='%s', depth='%" PRId32 "'",
            tile_key.ToHereTile().c_str(), kAggregateQuadTreeDepth);
      }
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_DEBUG_F(
          kLogTag, "GetQuadTreeIndexForTile not found in cache, tile='%s'",
          tile_key.ToHereTile().c_str());
      return client::ApiError::NotFound(
          "CacheOnly: resource not found in cache");
    }
  }

  // Quad tree data not found in the cache, or not all the requested
  // additional fields are present in the cache. Do a network request.
  auto query_api = lookup_client_.LookupApi(
      "query", "v1", static_cast<client::FetchOptions>(fetch_option), context);

  if (!query_api.IsSuccessful()) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "GetQuadTreeIndexForTile LookupApi failed, "
                          "hrn='%s', service='query', version='v1'",
                          catalog_.ToString().c_str());
    return query_api.GetError();
  }

  auto quadtree_response = QueryApi::QuadTreeIndex(
      query_api.GetResult(), layer_id_, root_tile_here, version,
      kAggregateQuadTreeDepth, std::move(additional_fields),
      request.GetBillingTag(), context);

  if (quadtree_response.status != olp::http::HttpStatusCode::OK) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "GetQuadTreeIndexForTile QuadTreeIndex failed, "
                          "hrn='%s', layer='%s', root='%s', "
                          "version='%" PRId64 "', depth='%" PRId32 "'",
                          catalog_.ToString().c_str(), layer_id_.c_str(),
                          root_tile_here.c_str(), version.get_value_or(-1),
                          kAggregateQuadTreeDepth);
    return QuadTreeIndexResponse(
        client::ApiError(quadtree_response.status,
                         quadtree_response.response.str()),
        quadtree_response.GetNetworkStatistics());
  }

  QuadTreeIndex tree(root_tile_key, kAggregateQuadTreeDepth,
                     quadtree_response.response);
  if (tree.IsNull()) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "GetQuadTreeIndexForTile QuadTreeIndex failed, hrn='%s', layer='%s', "
        "root='%s', version='%" PRId64 "', depth='%" PRId32 "'",
        catalog_.ToString().c_str(), layer_id_.c_str(), root_tile_here.c_str(),
        version.get_value_or(-1), kAggregateQuadTreeDepth);
    return QuadTreeIndexResponse(
        client::ApiError::Unknown("Failed to parse quad tree response"),
        quadtree_response.GetNetworkStatistics());
  }

  if (fetch_option != OnlineOnly) {
    cache_.Put(root_tile_key, kAggregateQuadTreeDepth, tree, version);
  }

  return QuadTreeIndexResponse(std::move(tree),
                               quadtree_response.GetNetworkStatistics());
}

PartitionResponse PartitionsRepository::GetAggregatedTile(
    TileRequest request, boost::optional<int64_t> version,
    client::CancellationContext context) {
  auto quad_tree_response = GetQuadTreeIndexForTile(request, version, context);
  if (!quad_tree_response.IsSuccessful()) {
    return PartitionResponse(quad_tree_response.GetError(),
                             quad_tree_response.GetPayload());
  }

  // When the parent tile is too far away, we iterate up and download metadata
  // for parent tiles until we cover aggregated tile root as a subquad. This
  // is needed for the users who need to access the aggregated tile root
  // directly. Else, we can't find it in cache.
  if (request.GetFetchOption() != FetchOptions::CacheOnly) {
    const auto& result = quad_tree_response.GetResult();
    auto index_data = result.Find(request.GetTileKey(), true);
    if (index_data) {
      const auto& aggregated_tile_key = index_data.value().tile_key;
      auto root = result.GetRootTile();
      while (root.Level() > aggregated_tile_key.Level()) {
        auto parent = root.Parent();
        request.WithTileKey(parent);
        // Ignore result for now
        GetQuadTreeIndexForTile(request, version, context);
        root = parent.ChangedLevelBy(-kAggregateQuadTreeDepth);
      }
    }
  }

  auto partition = FindPartition(quad_tree_response.GetResult(), request, true);
  if (!partition) {
    return PartitionResponse(
        client::ApiError::NotFound("Tile or its closest ancestors not found"),
        quad_tree_response.GetPayload());
  }
  return PartitionResponse(std::move(*partition),
                           quad_tree_response.GetPayload());
}

PartitionResponse PartitionsRepository::GetTile(
    const TileRequest& request, boost::optional<int64_t> version,
    client::CancellationContext context,
    boost::optional<std::vector<std::string>> additional_fields) {
  auto quad_tree_response = GetQuadTreeIndexForTile(
      request, version, std::move(context), std::move(additional_fields));
  if (!quad_tree_response.IsSuccessful()) {
    return PartitionResponse(quad_tree_response.GetError(),
                             quad_tree_response.GetPayload());
  }

  auto partition =
      FindPartition(quad_tree_response.GetResult(), request, false);
  if (!partition) {
    return PartitionResponse(
        client::ApiError::NotFound("Tile or its closest ancestors not found"),
        quad_tree_response.GetPayload());
  }
  return PartitionResponse(std::move(*partition),
                           quad_tree_response.GetPayload());
}

QueryApi::PartitionsExtendedResponse
PartitionsRepository::QueryPartitionsInBatches(
    const client::OlpClient& client,
    const PartitionsRequest::PartitionIds& partitions,
    boost::optional<std::int64_t> version,
    const PartitionsRequest::AdditionalFields& additional_fields,
    boost::optional<std::string> billing_tag,
    client::CancellationContext context) {
  std::vector<model::Partition> aggregated_partitions;
  aggregated_partitions.reserve(partitions.size());

  client::NetworkStatistics aggregated_network_statistics;

  PartitionsRequest::PartitionIds batch;
  batch.reserve(kQueryRequestLimit);

  for (size_t i = 0; i < partitions.size(); i += kQueryRequestLimit) {
    batch.clear();

    std::copy(partitions.begin() + i,
              partitions.begin() +
                  std::min(partitions.size(), i + kQueryRequestLimit),
              std::back_inserter(batch));

    auto query_response =
        QueryApi::GetPartitionsbyId(client, layer_id_, batch, version,
                                    additional_fields, billing_tag, context);
    if (!query_response) {
      return query_response.GetError();
    }

    auto partitions = query_response.MoveResult();
    auto& mutable_partitions = partitions.GetMutablePartitions();
    std::move(mutable_partitions.begin(), mutable_partitions.end(),
              std::back_inserter(aggregated_partitions));

    aggregated_network_statistics += query_response.GetPayload();
  }

  model::Partitions result_partitions;
  result_partitions.GetMutablePartitions().swap(aggregated_partitions);

  return QueryApi::PartitionsExtendedResponse(std::move(result_partitions),
                                              aggregated_network_statistics);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
