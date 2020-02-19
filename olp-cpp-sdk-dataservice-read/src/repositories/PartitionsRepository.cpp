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

namespace olp {
namespace dataservice {
namespace read {
namespace repository {
using namespace olp::client;

namespace {
constexpr auto kLogTag = "PartitionsRepository";

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
    client::OlpClientSettings settings, boost::optional<time_t> expiry) {
  auto fetch_option = request.GetFetchOption();
  std::chrono::seconds timeout{settings.retry_settings.timeout};

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

  auto query_api =
      ApiClientLookup::LookupApi(catalog, cancellation_context, "metadata",
                                 "v1", fetch_option, std::move(settings));

  if (!query_api.IsSuccessful()) {
    return query_api.GetError();
  }

  auto metadata_response = MetadataApi::GetPartitions(
      query_api.GetResult(), layer, request.GetVersion(), boost::none,
      boost::none, request.GetBillingTag(), cancellation_context);

  if (metadata_response.IsSuccessful()) {
    OLP_SDK_LOG_INFO_F(kLogTag, "put '%s' to cache",
                       request.CreateKey(layer).c_str());
    repository.Put(request, metadata_response.GetResult(), layer, expiry, true);
  } else {
    const auto& error = metadata_response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_INFO_F(kLogTag, "clear '%s' cache",
                         request.CreateKey(layer).c_str());
      repository.Clear(layer);
    }
  }

  return metadata_response;
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
      client, layer, partitions, version, boost::none,
      data_request.GetBillingTag(), cancellation_context);

  if (query_response.IsSuccessful()) {
    OLP_SDK_LOG_INFO_F(kLogTag, "put '%s' to cache",
                       data_request.CreateKey(layer).c_str());
    repository.Put(partition_request, query_response.GetResult(), layer,
                   boost::none /* TODO: expiration */);
  } else {
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

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
