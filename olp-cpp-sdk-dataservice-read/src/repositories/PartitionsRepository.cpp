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

#include <utility>

#include <olp/core/client/Condition.h>
#include <olp/core/logging/Log.h>
#include "ApiClientLookup.h"
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

// Needed to avoid endless warnings from GetVersion/WithVersion
#include <olp/core/porting/warning_disable.h>
PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

namespace {
namespace read = olp::dataservice::read;
namespace model = olp::dataservice::read::model;
namespace repository = olp::dataservice::read::repository;

constexpr auto kLogTag = "PartitionsRepository";
constexpr uint32_t kMaxQuadTreeIndexDepth = 4u;
constexpr auto kAggregateQuadTreeDepth = 4;

using LayerVersionReponse =
    olp::client::ApiResponse<int64_t, olp::client::ApiError>;
using LayerVersionCallback = std::function<void(LayerVersionReponse)>;

olp::client::ApiResponse<boost::optional<time_t>, olp::client::ApiError>
TtlForLayer(const std::vector<olp::dataservice::read::model::Layer>& layers,
            const std::string& layer_id) {
  for (const auto& layer : layers) {
    if (layer.GetId() == layer_id) {
      boost::optional<time_t> expiry;
      if (layer.GetTtl()) {
        expiry = *layer.GetTtl() / 1000;
      }
      return expiry;
    }
  }

  return olp::client::ApiError(olp::client::ErrorCode::NotFound,
                               "Layer specified doesn't exist");
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
    return {{olp::client::ErrorCode::NotFound,
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

  auto catalog_request = CatalogRequest()
                             .WithBillingTag(request.GetBillingTag())
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
  const auto key = request.CreateKey(layer);

  repository::PartitionsCacheRepository repository(
      catalog, settings.cache, settings.default_cache_expiration);

  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    auto cached_partitions = repository.Get(request, layer);
    if (cached_partitions) {
      OLP_SDK_LOG_DEBUG_F(kLogTag,
                          "GetPartitions found in cache, hrn='%s', key='%s'",
                          catalog.ToCatalogHRNString().c_str(), key.c_str());
      return cached_partitions.get();
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(kLogTag,
                         "GetPartitions not found in cache, hrn='%s', key='%s'",
                         catalog.ToCatalogHRNString().c_str(), key.c_str());
      return {{client::ErrorCode::NotFound,
               "CacheOnly: resource not found in cache"}};
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
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "GetPartitions put to cache, hrn='%s', key='%s'",
                        catalog.ToCatalogHRNString().c_str(), key.c_str());
    repository.Put(request, response.GetResult(), layer, expiry,
                   is_layer_metadata);
  }
  if (!response.IsSuccessful()) {
    const auto& error = response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag,
          "GetPartitions 403 received, remove from cache, hrn='%s', key='%s'",
          catalog.ToCatalogHRNString().c_str(), key.c_str());
      repository.Clear(layer);
    }
  }

  return response;
}

PartitionsResponse PartitionsRepository::GetPartitionById(
    const client::HRN& catalog, const std::string& layer,
    boost::optional<int64_t> version,
    client::CancellationContext cancellation_context,
    const DataRequest& data_request, client::OlpClientSettings settings) {
  const auto& partition_id = data_request.GetPartitionId();
  if (!partition_id) {
    return {{client::ErrorCode::PreconditionFailed, "Partition Id is missing"}};
  }

  auto fetch_option = data_request.GetFetchOption();

  const auto request_key =
      catalog.ToString() + data_request.CreateKey(layer, version);

  NamedMutex mutex(request_key);
  std::unique_lock<repository::NamedMutex> lock(mutex, std::defer_lock);

  // If we are not planning to go online or access the cache, do not lock.
  if (fetch_option != CacheOnly && fetch_option != OnlineOnly) {
    lock.lock();
  }

  std::chrono::seconds timeout{settings.retry_settings.timeout};
  repository::PartitionsCacheRepository repository(
      catalog, settings.cache, settings.default_cache_expiration);
  const auto key = data_request.CreateKey(layer, version);

  const std::vector<std::string> partitions{partition_id.value()};
  auto partition_request =
      PartitionsRequest().WithVersion(version).WithBillingTag(
          data_request.GetBillingTag());

  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    auto cached_partitions =
        repository.Get(partition_request, partitions, layer);
    if (cached_partitions.GetPartitions().size() == partitions.size()) {
      OLP_SDK_LOG_DEBUG_F(kLogTag,
                          "GetPartitionById found in cache, hrn='%s', key='%s'",
                          catalog.ToCatalogHRNString().c_str(), key.c_str());
      return cached_partitions;
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "GetPartitionById not found in cache, hrn='%s', key='%s'",
          catalog.ToCatalogHRNString().c_str(), key.c_str());
      return {{client::ErrorCode::NotFound,
               "CacheOnly: resource not found in cache"}};
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
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "GetPartitionById put to cache, hrn='%s', key='%s'",
                        catalog.ToCatalogHRNString().c_str(), key.c_str());
    repository.Put(partition_request, query_response.GetResult(), layer,
                   boost::none);
  } else if (!query_response.IsSuccessful()) {
    const auto& error = query_response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "GetPartitionById 403 received, remove from cache, "
                            "hrn='%s', key='%s'",
                            catalog.ToCatalogHRNString().c_str(), key.c_str());
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
QuadTreeIndexResponse PartitionsRepository::GetQuadTreeIndexForTile(
    const client::HRN& catalog, const std::string& layer,
    client::CancellationContext context, const TileRequest& request,
    boost::optional<int64_t> version,
    const client::OlpClientSettings& settings) {
  auto fetch_option = request.GetFetchOption();
  const auto& tile_key = request.GetTileKey();

  const auto& root_tile_key = tile_key.ChangedLevelBy(-kAggregateQuadTreeDepth);
  const auto root_tile_here = root_tile_key.ToHereTile();

  NamedMutex mutex(catalog.ToString() + layer + root_tile_here + "Index");
  std::unique_lock<NamedMutex> lock(mutex, std::defer_lock);

  // If we are not planning to go online or access the cache, do not lock.
  if (fetch_option != CacheOnly && fetch_option != OnlineOnly) {
    lock.lock();
  }

  // Look for QuadTree covering the tile in the cache
  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    read::QuadTreeIndex cached_tree;
    if (FindQuadTree(catalog, settings, layer, version, tile_key,
                     cached_tree)) {
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
  auto query_api = ApiClientLookup::LookupApi(
      catalog, context, "query", "v1", request.GetFetchOption(), settings);

  if (!query_api.IsSuccessful()) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "GetQuadTreeIndexForTile LookupApi failed, "
                          "hrn='%s', service='query', version='v1'",
                          catalog.ToString().c_str());
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
                          catalog.ToString().c_str(), layer.c_str(),
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
        catalog.ToString().c_str(), layer.c_str(), root_tile_here.c_str(),
        version.get_value_or(-1), kAggregateQuadTreeDepth);
    return {{client::ErrorCode::Unknown, "Failed to parse quad tree response"}};
  }

  if (fetch_option != OnlineOnly) {
    repository::PartitionsCacheRepository repository(
        catalog, settings.cache, settings.default_cache_expiration);
    repository.Put(layer, root_tile_key, kAggregateQuadTreeDepth, tree,
                   version);
  }
  return {std::move(tree)};
}

PartitionResponse PartitionsRepository::GetAggregatedTile(
    const client::HRN& catalog, const std::string& layer,
    client::CancellationContext cancellation_context,
    const TileRequest& request, boost::optional<int64_t> version,
    const client::OlpClientSettings& settings) {
  auto quad_tree_response = GetQuadTreeIndexForTile(
      catalog, layer, cancellation_context, request, version, settings);
  if (!quad_tree_response.IsSuccessful()) {
    return quad_tree_response.GetError();
  }
  return FindPartition(quad_tree_response.GetResult(), request, true);
}

PartitionResponse PartitionsRepository::GetTile(
    const client::HRN& catalog, const std::string& layer,
    client::CancellationContext cancellation_context,
    const TileRequest& request, boost::optional<int64_t> version,
    const client::OlpClientSettings& settings) {
  auto quad_tree_response = GetQuadTreeIndexForTile(
      catalog, layer, cancellation_context, request, version, settings);
  if (!quad_tree_response.IsSuccessful()) {
    return quad_tree_response.GetError();
  }
  return FindPartition(quad_tree_response.GetResult(), request, false);
}
PORTING_POP_WARNINGS()

bool PartitionsRepository::FindQuadTree(
    const client::HRN& catalog, const client::OlpClientSettings& settings,
    const std::string& layer, boost::optional<int64_t> version,
    const olp::geo::TileKey& tile_key, read::QuadTreeIndex& tree) {
  repository::PartitionsCacheRepository repository(
      catalog, settings.cache, settings.default_cache_expiration);
  auto max_depth =
      std::min<std::uint32_t>(tile_key.Level(), kMaxQuadTreeIndexDepth);
  for (int i = max_depth; i >= 0; --i) {
    const auto& root_tile_key = tile_key.ChangedLevelBy(-i);
    QuadTreeIndex cached_tree;
    if (repository.Get(layer, root_tile_key, kAggregateQuadTreeDepth, version,
                       cached_tree)) {
      OLP_SDK_LOG_DEBUG_F(kLogTag,
                          "FindQuadTreeInCache found in cache, tile='%s', "
                          "root='%s', depth='%" PRId32 "'",
                          tile_key.ToHereTile().c_str(),
                          root_tile_key.ToHereTile().c_str(),
                          kAggregateQuadTreeDepth);
      tree = std::move(cached_tree);

      return true;
    }
  }

  return false;
}
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
