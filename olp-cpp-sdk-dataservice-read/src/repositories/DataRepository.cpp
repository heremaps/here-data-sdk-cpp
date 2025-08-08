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

#include "DataRepository.h"

#include <string>
#include <utility>

#include <olp/core/client/Condition.h>
#include <olp/core/logging/Log.h>
#include "DataCacheRepository.h"
#include "PartitionsRepository.h"
#include "generated/api/BlobApi.h"
#include "generated/api/VolatileBlobApi.h"
#include "olp/dataservice/read/CatalogRequest.h"
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
                               client::ApiLookupClient client,
                               NamedMutexStorage storage)
    : catalog_(std::move(catalog)),
      settings_(std::move(settings)),
      lookup_client_(std::move(client)),
      storage_(std::move(storage)) {}

DataResponse DataRepository::GetVersionedTile(
    const std::string& layer_id, const TileRequest& request, int64_t version,
    client::CancellationContext context) {
  PartitionsRepository repository(catalog_, layer_id, settings_, lookup_client_,
                                  storage_);
  auto response = repository.GetTile(request, version, context, {});

  auto network_statistics = response.GetPayload();

  if (!response.IsSuccessful()) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "GetVersionedDataTile partition request failed, hrn='%s', key='%s'",
        catalog_.ToCatalogHRNString().c_str(),
        request.CreateKey(layer_id).c_str());
    return {response.GetError(), network_statistics};
  }

  // Get the data using a data handle for requested tile
  const auto& partition = response.GetResult();

  auto data_response =
      GetBlobData(layer_id, kBlobService, partition, request.GetFetchOption(),
                  request.GetBillingTag(), std::move(context),
                  settings_.propagate_all_cache_errors);
  network_statistics += data_response.GetPayload();

  if (data_response) {
    return {data_response.MoveResult(), network_statistics};
  } else {
    return {data_response.GetError(), network_statistics};
  }
}

BlobApi::DataResponse DataRepository::GetVersionedData(
    const std::string& layer_id, const DataRequest& request, int64_t version,
    client::CancellationContext context, const bool fail_on_cache_error) {
  if (request.GetDataHandle() && request.GetPartitionId()) {
    return client::ApiError::PreconditionFailed(
        "Both data handle and partition id specified");
  }

  client::NetworkStatistics network_statistics;

  model::Partition partition;

  if (request.GetDataHandle()) {
    partition.SetDataHandle(*request.GetDataHandle());
  } else {
    // get data handle for a partition to be queried
    PartitionsRepository repository(catalog_, layer_id, settings_,
                                    lookup_client_, storage_);
    auto partitions_response =
        repository.GetPartitionById(request, version, context);

    network_statistics = partitions_response.GetPayload();

    if (!partitions_response.IsSuccessful()) {
      return DataResponse(partitions_response.GetError(), network_statistics);
    }

    auto partitions_result = partitions_response.MoveResult();
    auto& partitions = partitions_result.GetMutablePartitions();
    if (partitions.empty()) {
      OLP_SDK_LOG_INFO_F(
          kLogTag,
          "GetVersionedData partition %s not found, hrn='%s', key='%s'",
          request.GetPartitionId() ? request.GetPartitionId()->c_str()
                                   : "<none>",
          catalog_.ToCatalogHRNString().c_str(),
          request.CreateKey(layer_id, version).c_str());

      return DataResponse(client::ApiError::NotFound("Partition not found"),
                          network_statistics);
    }

    partition = std::move(partitions.front());
  }

  // finally get the data using a data handle
  auto data_response = GetBlobData(
      layer_id, kBlobService, partition, request.GetFetchOption(),
      request.GetBillingTag(), std::move(context), fail_on_cache_error);

  network_statistics += data_response.GetPayload();

  if (data_response) {
    return DataResponse(data_response.MoveResult(), network_statistics);
  } else {
    return DataResponse(data_response.GetError(), network_statistics);
  }
}

BlobApi::DataResponse DataRepository::GetBlobData(
    const std::string& layer, const std::string& service,
    const model::Partition& partition, FetchOptions fetch_option,
    const porting::optional<std::string>& billing_tag,
    client::CancellationContext context, const bool fail_on_cache_error) {
  const auto& data_handle = partition.GetDataHandle();
  if (data_handle.empty()) {
    return client::ApiError::PreconditionFailed("Data handle is missing");
  }

  NamedMutex mutex(storage_,
                   catalog_.ToString() + layer + partition.GetDataHandle(),
                   context);
  std::unique_lock<NamedMutex> lock(mutex, std::defer_lock);

  // If we are not planning to go online or access the cache, do not lock.
  if (fetch_option != CacheOnly && fetch_option != OnlineOnly) {
    lock.lock();
  }

  repository::DataCacheRepository repository(
      catalog_, settings_.cache, settings_.default_cache_expiration);

  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    auto cached_data = repository.Get(layer, data_handle);
    if (cached_data) {
      OLP_SDK_LOG_TRACE_F(
          kLogTag, "GetBlobData found in cache, hrn='%s', key='%s'",
          catalog_.ToCatalogHRNString().c_str(), data_handle.c_str());
      return cached_data.value();
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "GetBlobData not found in cache, hrn='%s', key='%s'",
          catalog_.ToCatalogHRNString().c_str(), data_handle.c_str());
      return client::ApiError::NotFound(
          "CacheOnly: resource not found in cache");
    }
  }

  // Check if other threads have faced an error.
  const auto optional_error = mutex.GetError();
  if (optional_error) {
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "Found error in NamedMutex, aborting, hrn='%s', "
                        "key='%s', error='%s'",
                        catalog_.ToCatalogHRNString().c_str(),
                        data_handle.c_str(),
                        optional_error->GetMessage().c_str());
    return *optional_error;
  }

  auto storage_api_lookup = lookup_client_.LookupApi(
      service, "v1", static_cast<client::FetchOptions>(fetch_option), context);

  if (!storage_api_lookup.IsSuccessful()) {
    // Store an error to share it with other threads.
    mutex.SetError(storage_api_lookup.GetError());

    return storage_api_lookup.GetError();
  }

  BlobApi::DataResponse storage_response;

  if (service == kBlobService) {
    storage_response =
        BlobApi::GetBlob(storage_api_lookup.GetResult(), layer, partition,
                         billing_tag, olp::porting::none, context);
  } else {
    auto volatile_blob =
        VolatileBlobApi::GetVolatileBlob(storage_api_lookup.GetResult(), layer,
                                         data_handle, billing_tag, context);
    storage_response = BlobApi::DataResponse(volatile_blob.MoveResult());
  }

  if (storage_response.IsSuccessful() && fetch_option != OnlineOnly) {
    const auto put_result =
        repository.Put(storage_response.GetResult(), layer, data_handle);
    if (!put_result.IsSuccessful() && fail_on_cache_error) {
      OLP_SDK_LOG_ERROR_F(kLogTag,
                          "Failed to write data to cache, hrn='%s', "
                          "layer='%s', data_handle='%s'",
                          catalog_.ToCatalogHRNString().c_str(), layer.c_str(),
                          data_handle.c_str());
      return put_result.GetError();
    }
  }

  if (!storage_response.IsSuccessful()) {
    const auto& error = storage_response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag,
          "GetBlobData 403 received, remove from cache, hrn='%s', key='%s'",
          catalog_.ToCatalogHRNString().c_str(), data_handle.c_str());
      repository.Clear(layer, data_handle);
    }

    // Store an error to share it with other threads.
    mutex.SetError(storage_response.GetError());
  }

  return storage_response;
}

BlobApi::DataResponse DataRepository::GetVolatileData(
    const std::string& layer_id, const DataRequest& request,
    client::CancellationContext context, const bool fail_on_cache_error) {
  if (request.GetDataHandle() && request.GetPartitionId()) {
    return client::ApiError::PreconditionFailed(
        "Both data handle and partition id specified");
  }

  model::Partition partition;

  if (request.GetDataHandle()) {
    partition.SetDataHandle(*request.GetDataHandle());
  } else {
    PartitionsRepository repository(catalog_, layer_id, settings_,
                                    lookup_client_, storage_);
    auto partitions_response =
        repository.GetPartitionById(request, olp::porting::none, context);

    if (!partitions_response.IsSuccessful()) {
      return partitions_response.GetError();
    }

    auto partitions_result = partitions_response.MoveResult();
    auto& partitions = partitions_result.GetMutablePartitions();

    if (partitions.empty()) {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "GetVolatileData partition %s not found, hrn='%s', key='%s'",
          request.GetPartitionId() ? request.GetPartitionId()->c_str()
                                   : "<none>",
          catalog_.ToCatalogHRNString().c_str(),
          request.CreateKey(layer_id, olp::porting::none).c_str());

      return client::ApiError::NotFound("Partition not found");
    }

    partition = std::move(partitions.front());
  }

  return GetBlobData(layer_id, kVolatileBlobService, partition,
                     request.GetFetchOption(), request.GetBillingTag(),
                     std::move(context), fail_on_cache_error);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
