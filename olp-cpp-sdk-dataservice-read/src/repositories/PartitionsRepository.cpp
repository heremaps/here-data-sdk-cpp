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

#include <algorithm>
#include <utility>

#include <olp/core/client/Condition.h>
#include <olp/core/logging/Log.h>
#include "CatalogRepository.h"
#include "NamedMutex.h"
#include "PartitionsCacheRepository.h"
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

using LayerVersionReponse = client::ApiResponse<int64_t, client::ApiError>;
using LayerVersionCallback = std::function<void(LayerVersionReponse)>;

client::ApiResponse<boost::optional<time_t>, client::ApiError> TtlForLayer(
    const std::vector<model::Layer>& layers, const std::string& layer_id) {
  auto layer_it = std::find_if(
      std::begin(layers), std::end(layers),
      [&](const model::Layer& layer) { return layer.GetId() == layer_id; });

  if (layer_it == std::end(layers)) {
    return client::ApiError(client::ErrorCode::NotFound,
                            "Layer specified doesn't exist");
  }

  auto ttl = layer_it->GetTtl();

  return ttl ? boost::make_optional<time_t>(ttl.value() / 1000) : boost::none;
}

repository::PartitionResponse FindPartition(
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
    return {{client::ErrorCode::NotFound,
             "Tile or its closest ancestors not found"}};
  }

  const auto index_data = found_index_data.get();
  const auto found_tile_key = index_data.tile_key;
  const auto data_handle = index_data.data_handle;

  auto aggregated_partition = model::Partition();
  aggregated_partition.SetDataHandle(data_handle);
  aggregated_partition.SetPartition(found_tile_key.ToHereTile());

  return std::move(aggregated_partition);
}
}  // namespace

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

PartitionsRepository::PartitionsRepository(client::HRN catalog,
                                           client::OlpClientSettings settings,
                                           client::ApiLookupClient client)
    : catalog_(std::move(catalog)),
      settings_(std::move(settings)),
      lookup_client_(std::move(client)) {}

PartitionsResponse PartitionsRepository::GetVersionedPartitions(
    std::string layer, const PartitionsRequest& request, int64_t version,
    client::CancellationContext context) {
  return GetPartitions(std::move(layer), request, version, std::move(context));
}

PartitionsResponse PartitionsRepository::GetVolatilePartitions(
    std::string layer, const PartitionsRequest& request,
    client::CancellationContext context) {
  auto catalog_request = CatalogRequest()
                             .WithBillingTag(request.GetBillingTag())
                             .WithFetchOption(request.GetFetchOption());

  CatalogRepository repository(catalog_, settings_, lookup_client_);
  auto catalog_response = repository.GetCatalog(catalog_request, context);

  if (!catalog_response.IsSuccessful()) {
    return catalog_response.GetError();
  }

  auto expiry_response =
      TtlForLayer(catalog_response.GetResult().GetLayers(), layer);
  if (!expiry_response.IsSuccessful()) {
    return expiry_response.GetError();
  }

  return GetPartitions(std::move(layer), request, boost::none, context,
                       expiry_response.MoveResult());
}

PartitionsResponse PartitionsRepository::GetPartitions(
    std::string layer, const PartitionsRequest& request,
    boost::optional<std::int64_t> version, client::CancellationContext context,
    boost::optional<time_t> expiry) {
  auto fetch_option = request.GetFetchOption();
  const auto key = request.CreateKey(layer);

  repository::PartitionsCacheRepository repository(
      catalog_, settings_.cache, settings_.default_cache_expiration);

  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    auto cached_partitions = repository.Get(request, layer, version);
    if (cached_partitions) {
      OLP_SDK_LOG_DEBUG_F(kLogTag,
                          "GetPartitions found in cache, hrn='%s', key='%s'",
                          catalog_.ToCatalogHRNString().c_str(), key.c_str());
      return cached_partitions.get();
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(kLogTag,
                         "GetPartitions not found in cache, hrn='%s', key='%s'",
                         catalog_.ToCatalogHRNString().c_str(), key.c_str());
      return {{client::ErrorCode::NotFound,
               "CacheOnly: resource not found in cache"}};
    }
  }

  PartitionsResponse response;
  const auto& partition_ids = request.GetPartitionIds();

  if (partition_ids.empty()) {
    auto metadata_api = lookup_client_.LookupApi(
        "metadata", "v1", static_cast<client::FetchOptions>(fetch_option),
        context);

    if (!metadata_api.IsSuccessful()) {
      return metadata_api.GetError();
    }

    response = MetadataApi::GetPartitions(
        metadata_api.GetResult(), layer, version, request.GetAdditionalFields(),
        boost::none, request.GetBillingTag(), context);
  } else {
    auto query_api = lookup_client_.LookupApi(
        "query", "v1", static_cast<client::FetchOptions>(fetch_option),
        context);

    if (!query_api.IsSuccessful()) {
      return query_api.GetError();
    }

    response = QueryApi::GetPartitionsbyId(
        query_api.GetResult(), layer, partition_ids, version,
        request.GetAdditionalFields(), request.GetBillingTag(), context);
  }

  // Save all partitions only when downloaded via metadata API
  const bool is_layer_metadata = partition_ids.empty();
  if (response.IsSuccessful() && fetch_option != OnlineOnly) {
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "GetPartitions put to cache, hrn='%s', key='%s'",
                        catalog_.ToCatalogHRNString().c_str(), key.c_str());
    repository.Put(response.GetResult(), layer, version, expiry,
                   is_layer_metadata);
  }
  if (!response.IsSuccessful()) {
    const auto& error = response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag,
          "GetPartitions 403 received, remove from cache, hrn='%s', key='%s'",
          catalog_.ToCatalogHRNString().c_str(), key.c_str());
      repository.Clear(layer);
    }
  }

  return response;
}

PartitionsResponse PartitionsRepository::GetPartitionById(
    const std::string& layer, boost::optional<int64_t> version,
    const DataRequest& data_request, client::CancellationContext context) {
  const auto& partition_id = data_request.GetPartitionId();
  if (!partition_id) {
    return {{client::ErrorCode::PreconditionFailed, "Partition Id is missing"}};
  }

  auto fetch_option = data_request.GetFetchOption();

  const auto request_key =
      catalog_.ToString() + data_request.CreateKey(layer, version);

  NamedMutex mutex(request_key);
  std::unique_lock<repository::NamedMutex> lock(mutex, std::defer_lock);

  // If we are not planning to go online or access the cache, do not lock.
  if (fetch_option != CacheOnly && fetch_option != OnlineOnly) {
    lock.lock();
  }

  std::chrono::seconds timeout{settings_.retry_settings.timeout};
  repository::PartitionsCacheRepository repository(
      catalog_, settings_.cache, settings_.default_cache_expiration);
  const auto key = data_request.CreateKey(layer, version);

  const std::vector<std::string> partitions{partition_id.value()};
  auto partition_request =
      PartitionsRequest().WithBillingTag(data_request.GetBillingTag());

  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    auto cached_partitions = repository.Get(partitions, layer, version);
    if (cached_partitions.GetPartitions().size() == partitions.size()) {
      OLP_SDK_LOG_DEBUG_F(kLogTag,
                          "GetPartitionById found in cache, hrn='%s', key='%s'",
                          catalog_.ToCatalogHRNString().c_str(), key.c_str());
      return cached_partitions;
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "GetPartitionById not found in cache, hrn='%s', key='%s'",
          catalog_.ToCatalogHRNString().c_str(), key.c_str());
      return {{client::ErrorCode::NotFound,
               "CacheOnly: resource not found in cache"}};
    }
  }

  auto query_api = lookup_client_.LookupApi(
      "query", "v1", static_cast<client::FetchOptions>(fetch_option), context);

  if (!query_api.IsSuccessful()) {
    return query_api.GetError();
  }

  const client::OlpClient& client = query_api.GetResult();

  PartitionsResponse query_response =
      QueryApi::GetPartitionsbyId(client, layer, partitions, version, {},
                                  data_request.GetBillingTag(), context);

  if (query_response.IsSuccessful() && fetch_option != OnlineOnly) {
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "GetPartitionById put to cache, hrn='%s', key='%s'",
                        catalog_.ToCatalogHRNString().c_str(), key.c_str());
    repository.Put(query_response.GetResult(), layer, version, boost::none);
  } else if (!query_response.IsSuccessful()) {
    const auto& error = query_response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "GetPartitionById 403 received, remove from cache, "
                            "hrn='%s', key='%s'",
                            catalog_.ToCatalogHRNString().c_str(), key.c_str());
      // Delete partitions only but not the layer
      repository.ClearPartitions(partitions, layer, version);
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
    const std::string& layer, const TileRequest& request,
    boost::optional<int64_t> version, client::CancellationContext context) {
  auto fetch_option = request.GetFetchOption();
  const auto& tile_key = request.GetTileKey();

  const auto& root_tile_key = tile_key.ChangedLevelBy(-kAggregateQuadTreeDepth);
  const auto root_tile_here = root_tile_key.ToHereTile();

  NamedMutex mutex(catalog_.ToString() + layer + root_tile_here + "Index");
  std::unique_lock<NamedMutex> lock(mutex, std::defer_lock);

  // If we are not planning to go online or access the cache, do not lock.
  if (fetch_option != CacheOnly && fetch_option != OnlineOnly) {
    lock.lock();
  }

  // Look for QuadTree covering the tile in the cache
  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    read::QuadTreeIndex cached_tree;
    repository::PartitionsCacheRepository repository(
        catalog_, settings_.cache, settings_.default_cache_expiration);
    if (repository.FindQuadTree(layer, version, tile_key, cached_tree)) {
      OLP_SDK_LOG_DEBUG_F(kLogTag,
                          "GetQuadTreeIndexForTile found in cache, "
                          "tile='%s', depth='%" PRId32 "'",
                          tile_key.ToHereTile().c_str(),
                          kAggregateQuadTreeDepth);

      return {std::move(cached_tree)};
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "GetQuadTreeIndexForTile not found in cache, tile='%s'",
          tile_key.ToHereTile().c_str());
      return {{client::ErrorCode::NotFound,
               "CacheOnly: resource not found in cache"}};
    }
  }

  // quad tree data not found in the cache
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
      query_api.GetResult(), layer, root_tile_here, version,
      kAggregateQuadTreeDepth, boost::none, request.GetBillingTag(), context);

  if (quadtree_response.status != olp::http::HttpStatusCode::OK) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "GetQuadTreeIndexForTile QuadTreeIndex failed, "
                          "hrn='%s', layer='%s', root='%s', "
                          "version='%" PRId64 "', depth='%" PRId32 "'",
                          catalog_.ToString().c_str(), layer.c_str(),
                          root_tile_here.c_str(), version.get_value_or(-1),
                          kAggregateQuadTreeDepth);
    return {{quadtree_response.status, quadtree_response.response.str()}};
  }

  QuadTreeIndex tree(root_tile_key, kAggregateQuadTreeDepth,
                     quadtree_response.response);
  if (tree.IsNull()) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "GetQuadTreeIndexForTile QuadTreeIndex failed, hrn='%s', layer='%s', "
        "root='%s', version='%" PRId64 "', depth='%" PRId32 "'",
        catalog_.ToString().c_str(), layer.c_str(), root_tile_here.c_str(),
        version.get_value_or(-1), kAggregateQuadTreeDepth);
    return {{client::ErrorCode::Unknown, "Failed to parse quad tree response"}};
  }

  if (fetch_option != OnlineOnly) {
    repository::PartitionsCacheRepository repository(
        catalog_, settings_.cache, settings_.default_cache_expiration);
    repository.Put(layer, root_tile_key, kAggregateQuadTreeDepth, tree,
                   version);
  }
  return {std::move(tree)};
}

PartitionResponse PartitionsRepository::GetAggregatedTile(
    const std::string& layer, const TileRequest& request,
    boost::optional<int64_t> version, client::CancellationContext context) {
  auto quad_tree_response =
      GetQuadTreeIndexForTile(layer, request, version, context);
  if (!quad_tree_response.IsSuccessful()) {
    return quad_tree_response.GetError();
  }
  return FindPartition(quad_tree_response.GetResult(), request, true);
}

PartitionResponse PartitionsRepository::GetTile(
    const std::string& layer, const TileRequest& request,
    boost::optional<int64_t> version, client::CancellationContext context) {
  auto quad_tree_response =
      GetQuadTreeIndexForTile(layer, request, version, context);
  if (!quad_tree_response.IsSuccessful()) {
    return quad_tree_response.GetError();
  }

  return FindPartition(quad_tree_response.GetResult(), request, false);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
