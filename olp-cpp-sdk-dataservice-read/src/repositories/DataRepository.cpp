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

#include "DataRepository.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>

#include <olp/core/client/Condition.h>
#include <olp/core/logging/Log.h>
#include "CatalogRepository.h"
#include "DataCacheRepository.h"
#include "ExecuteOrSchedule.inl"
#include "NamedMutex.h"
#include "PartitionsCacheRepository.h"
#include "PartitionsRepository.h"
#include "generated/api/BlobApi.h"
#include "generated/api/VolatileBlobApi.h"
#include "olp/dataservice/read/CatalogRequest.h"
#include "olp/dataservice/read/CatalogVersionRequest.h"
#include "olp/dataservice/read/PartitionsRequest.h"
#include "olp/dataservice/read/TileRequest.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

namespace {
constexpr auto kLogTag = "DataRepository";
constexpr auto kBlobService = "blob";
constexpr auto kVolatileBlobService = "volatile-blob";
}  // namespace

DataRepository::DataRepository(client::HRN catalog,
                               client::OlpClientSettings settings,
                               client::ApiLookupClient client)
    : catalog_(std::move(catalog)),
      settings_(settings),
      lookup_client_(std::move(client)) {}

DataResponse DataRepository::GetVersionedTile(
    const std::string& layer_id, const TileRequest& request, int64_t version,
    client::CancellationContext context) {
  PartitionsRepository repository(catalog_, settings_, lookup_client_);
  auto response = repository.GetTile(layer_id, request, version, context);

  if (!response.IsSuccessful()) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "GetVersionedDataTile partition request failed, hrn='%s', key='%s'",
        catalog_.ToCatalogHRNString().c_str(),
        request.CreateKey(layer_id).c_str());
    return response.GetError();
  }

  // Get the data using a data handle for requested tile
  const auto& partition = response.GetResult();
  const auto data_request = DataRequest()
                                .WithDataHandle(partition.GetDataHandle())
                                .WithFetchOption(request.GetFetchOption());

  return GetBlobData(layer_id, kBlobService, data_request, context);
}

BlobApi::DataResponse DataRepository::GetVersionedData(
    const std::string& layer_id, const DataRequest& request, int64_t version,
    client::CancellationContext context) {
  if (request.GetDataHandle() && request.GetPartitionId()) {
    return {{client::ErrorCode::PreconditionFailed,
             "Both data handle and partition id specified"}};
  }

  auto blob_request = request;
  if (!request.GetDataHandle()) {
    // get data handle for a partition to be queried
    PartitionsRepository repository(catalog_, settings_, lookup_client_);
    auto partitions_response =
        repository.GetPartitionById(layer_id, version, request, context);

    if (!partitions_response.IsSuccessful()) {
      return partitions_response.GetError();
    }

    const auto& partitions = partitions_response.GetResult().GetPartitions();
    if (partitions.empty()) {
      OLP_SDK_LOG_INFO_F(
          kLogTag,
          "GetVersionedData partition %s not found, hrn='%s', key='%s'",
          request.GetPartitionId() ? request.GetPartitionId().get().c_str()
                                   : "<none>",
          catalog_.ToCatalogHRNString().c_str(),
          request.CreateKey(layer_id, version).c_str());

      return {{client::ErrorCode::NotFound, "Partition not found"}};
    }

    blob_request.WithDataHandle(partitions.front().GetDataHandle());
  }

  // finally get the data using a data handle
  return repository::DataRepository::GetBlobData(layer_id, kBlobService,
                                                 blob_request, context);
}

BlobApi::DataResponse DataRepository::GetBlobData(
    const std::string& layer, const std::string& service,
    const DataRequest& data_request, client::CancellationContext context) {
  auto fetch_option = data_request.GetFetchOption();
  const auto& data_handle = data_request.GetDataHandle();

  if (!data_handle) {
    return {{client::ErrorCode::PreconditionFailed, "Data handle is missing"}};
  }

  NamedMutex mutex(catalog_.ToString() + layer + *data_handle);
  std::unique_lock<NamedMutex> lock(mutex, std::defer_lock);

  // If we are not planning to go online or access the cache, do not lock.
  if (fetch_option != CacheOnly && fetch_option != OnlineOnly) {
    lock.lock();
  }

  repository::DataCacheRepository repository(
      catalog_, settings_.cache, settings_.default_cache_expiration);

  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    auto cached_data = repository.Get(layer, data_handle.value());
    if (cached_data) {
      OLP_SDK_LOG_DEBUG_F(
          kLogTag, "GetBlobData found in cache, hrn='%s', key='%s'",
          catalog_.ToCatalogHRNString().c_str(), data_handle->c_str());
      return cached_data.value();
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "GetBlobData not found in cache, hrn='%s', key='%s'",
          catalog_.ToCatalogHRNString().c_str(), data_handle->c_str());
      return {{client::ErrorCode::NotFound,
               "CacheOnly: resource not found in cache"}};
    }
  }

  auto storage_api_lookup = lookup_client_.LookupApi(
      service, "v1", static_cast<client::FetchOptions>(fetch_option), context);

  if (!storage_api_lookup.IsSuccessful()) {
    return storage_api_lookup.GetError();
  }

  BlobApi::DataResponse storage_response;

  if (service == kBlobService) {
    storage_response = BlobApi::GetBlob(
        storage_api_lookup.GetResult(), layer, data_handle.value(),
        data_request.GetBillingTag(), boost::none, context);
  } else {
    auto volatile_blob = VolatileBlobApi::GetVolatileBlob(
        storage_api_lookup.GetResult(), layer, data_handle.value(),
        data_request.GetBillingTag(), context);
    storage_response = BlobApi::DataResponse(volatile_blob.MoveResult());
  }

  if (storage_response.IsSuccessful() && fetch_option != OnlineOnly) {
    repository.Put(storage_response.GetResult(), layer, data_handle.value());
  }

  if (!storage_response.IsSuccessful()) {
    const auto& error = storage_response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag,
          "GetBlobData 403 received, remove from cache, hrn='%s', key='%s'",
          catalog_.ToCatalogHRNString().c_str(), data_handle->c_str());
      repository.Clear(layer, data_handle.value());
    }
  }

  return storage_response;
}

BlobApi::DataResponse DataRepository::GetVolatileData(
    const std::string& layer_id, const DataRequest& request,
    client::CancellationContext context) {
  if (request.GetDataHandle() && request.GetPartitionId()) {
    return {{client::ErrorCode::PreconditionFailed,
             "Both data handle and partition id specified"}};
  }

  auto blob_request = request;
  if (!request.GetDataHandle()) {
    PartitionsRepository repository(catalog_, settings_, lookup_client_);
    auto partitions_response =
        repository.GetPartitionById(layer_id, boost::none, request, context);

    if (!partitions_response.IsSuccessful()) {
      return partitions_response.GetError();
    }

    const auto& partitions = partitions_response.GetResult().GetPartitions();
    if (partitions.empty()) {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "GetVolatileData partition %s not found, hrn='%s', key='%s'",
          request.GetPartitionId() ? request.GetPartitionId().get().c_str()
                                   : "<none>",
          catalog_.ToCatalogHRNString().c_str(),
          request.CreateKey(layer_id, boost::none).c_str());

      return {{client::ErrorCode::NotFound, "Partition not found"}};
    }

    blob_request.WithDataHandle(partitions.front().GetDataHandle());
  }

  return GetBlobData(layer_id, kVolatileBlobService, blob_request, context);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
