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
#include "ApiClientLookup.h"
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

// Needed to avoid endless warnings from GetVersion/WithVersion
#include <olp/core/porting/warning_disable.h>
PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

namespace {
constexpr auto kLogTag = "DataRepository";
constexpr auto kBlobService = "blob";
constexpr auto kVolatileBlobService = "volatile-blob";
}  // namespace

DataResponse DataRepository::GetVersionedTile(
    const client::HRN& catalog, const std::string& layer_id,
    const TileRequest& request, int64_t version,
    client::CancellationContext context,
    const client::OlpClientSettings& settings) {
  auto response = PartitionsRepository::GetTile(catalog, layer_id, context,
                                                request, version, settings);

  if (!response.IsSuccessful()) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "GetVersionedDataTile partition request failed, hrn='%s', key='%s'",
        catalog.ToCatalogHRNString().c_str(),
        request.CreateKey(layer_id).c_str());
    return response.GetError();
  }

  // Get the data using a data handle for requested tile
  const auto& partition = response.GetResult();
  const auto data_request = DataRequest()
                                .WithDataHandle(partition.GetDataHandle())
                                .WithFetchOption(request.GetFetchOption());

  return repository::DataRepository::GetBlobData(
      catalog, layer_id, kBlobService, data_request, context, settings);
}

DataResponse DataRepository::GetVersionedData(
    const client::HRN& catalog, const std::string& layer_id, int64_t version,
    DataRequest request, client::CancellationContext context,
    const client::OlpClientSettings& settings) {
  if (request.GetDataHandle() && request.GetPartitionId()) {
    return {{client::ErrorCode::PreconditionFailed,
             "Both data handle and partition id specified"}};
  }

  if (!request.GetDataHandle()) {
    // get data handle for a partition to be queried
    auto partitions_response =
        repository::PartitionsRepository::GetPartitionById(
            catalog, layer_id, version, context, request, settings);

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
          catalog.ToCatalogHRNString().c_str(),
          request.CreateKey(layer_id, version).c_str());

      return {{client::ErrorCode::NotFound, "Partition not found"}};
    }

    request.WithDataHandle(partitions.front().GetDataHandle());
  }

  // finally get the data using a data handle
  return repository::DataRepository::GetBlobData(
      catalog, layer_id, kBlobService, request, context, settings);
}

DataResponse DataRepository::GetBlobData(
    const client::HRN& catalog, const std::string& layer,
    const std::string& service, const DataRequest& data_request,
    client::CancellationContext cancellation_context,
    const client::OlpClientSettings& settings) {
  auto fetch_option = data_request.GetFetchOption();
  const auto& data_handle = data_request.GetDataHandle();

  if (!data_handle) {
    return {{client::ErrorCode::PreconditionFailed, "Data handle is missing"}};
  }

  NamedMutex mutex(catalog.ToString() + layer + *data_handle);
  std::unique_lock<NamedMutex> lock(mutex, std::defer_lock);

  // If we are not planning to go online or access the cache, do not lock.
  if (fetch_option != CacheOnly && fetch_option != OnlineOnly) {
    lock.lock();
  }

  repository::DataCacheRepository repository(catalog, settings.cache,
                                             settings.default_cache_expiration);

  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    auto cached_data = repository.Get(layer, data_handle.value());
    if (cached_data) {
      OLP_SDK_LOG_DEBUG_F(
          kLogTag, "GetBlobData found in cache, hrn='%s', key='%s'",
          catalog.ToCatalogHRNString().c_str(), data_handle->c_str());
      return cached_data.value();
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "GetBlobData not found in cache, hrn='%s', key='%s'",
          catalog.ToCatalogHRNString().c_str(), data_handle->c_str());
      return {{client::ErrorCode::NotFound,
               "CacheOnly: resource not found in cache"}};
    }
  }

  auto blob_api = ApiClientLookup::LookupApi(
      catalog, cancellation_context, service, "v1", fetch_option, settings);

  if (!blob_api.IsSuccessful()) {
    return blob_api.GetError();
  }

  BlobApi::DataResponse blob_response;

  if (service == kBlobService) {
    blob_response = BlobApi::GetBlob(
        blob_api.GetResult(), layer, data_handle.value(),
        data_request.GetBillingTag(), boost::none, cancellation_context);
  } else {
    blob_response = VolatileBlobApi::GetVolatileBlob(
        blob_api.GetResult(), layer, data_handle.value(),
        data_request.GetBillingTag(), cancellation_context);
  }

  if (blob_response.IsSuccessful() && fetch_option != OnlineOnly) {
    repository.Put(blob_response.GetResult(), layer, data_handle.value());
  }

  if (!blob_response.IsSuccessful()) {
    const auto& error = blob_response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag,
          "GetBlobData 403 received, remove from cache, hrn='%s', key='%s'",
          catalog.ToCatalogHRNString().c_str(), data_handle->c_str());
      repository.Clear(layer, data_handle.value());
    }
  }

  return blob_response;
}

DataResponse DataRepository::GetVolatileData(
    const client::HRN& catalog, const std::string& layer_id,
    DataRequest request, client::CancellationContext context,
    const client::OlpClientSettings& settings) {
  if (request.GetDataHandle() && request.GetPartitionId()) {
    return {{client::ErrorCode::PreconditionFailed,
             "Both data handle and partition id specified"}};
  }

  if (!request.GetDataHandle()) {
    auto partitions_response =
        repository::PartitionsRepository::GetPartitionById(
            catalog, layer_id, boost::none, context, request, settings);

    if (!partitions_response.IsSuccessful()) {
      return partitions_response.GetError();
    }

    const auto& partitions = partitions_response.GetResult().GetPartitions();
    if (partitions.empty()) {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "GetVolatileData partition %s not found, hrn='%s', key='%s'",
          request.GetPartitionId() ? request.GetPartitionId().get().c_str()
                                   : "<none>",
          catalog.ToCatalogHRNString().c_str(),
          request.CreateKey(layer_id, boost::none).c_str());

      return {{client::ErrorCode::NotFound, "Partition not found"}};
    }

    request.WithDataHandle(partitions.front().GetDataHandle());
  }

  return repository::DataRepository::GetBlobData(
      catalog, layer_id, kVolatileBlobService, request, context, settings);
}

PORTING_POP_WARNINGS()
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
