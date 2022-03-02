/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include <olp/core/client/Condition.h>
#include <olp/core/logging/Log.h>
#include "CatalogRepository.h"
#include "DataCacheRepository.h"
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
                               client::ApiLookupClient client,
                               NamedMutexStorage storage)
    : catalog_(std::move(catalog)),
      settings_(std::move(settings)),
      lookup_client_(std::move(client)),
      storage_(std::move(storage)),
      partition_repository_(catalog_, "", settings_, lookup_client_),
      data_repository_(catalog_, settings_.cache,
                       settings_.default_cache_expiration) {}

DataResponse DataRepository::GetVersionedTile(
    const std::string& layer_id, const TileRequest& request, int64_t version,
    client::CancellationContext context) {
  PartitionsRepository repository(catalog_, layer_id, settings_, lookup_client_,
                                  storage_);
  auto response = repository.GetTile(request, version, context);

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

  return GetBlobData(layer_id, kBlobService, data_request, std::move(context));
}

client::CancellationToken DataRepository::GetVersionedTile(
    const std::string& layer_id, const TileRequest& request, int64_t version,
    std::function<void(DataResponse)> callback) {
  //  PartitionsRepository repository(catalog_, layer_id, settings_,
  //  lookup_client_,
  //                                  storage_);
  client::CancellationContext context;
  partition_repository_.set_layer_id(layer_id);
  auto response = partition_repository_.GetTile(request, version, context);

  if (!response.IsSuccessful()) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "GetVersionedDataTile partition request failed, hrn='%s', key='%s'",
        catalog_.ToCatalogHRNString().c_str(),
        request.CreateKey(layer_id).c_str());
    callback(response.GetError());
    return client::CancellationToken();
  }

  // Get the data using a data handle for requested tile
  const auto& partition = response.GetResult();
  const auto data_request = DataRequest()
                                .WithDataHandle(partition.GetDataHandle())
                                .WithFetchOption(request.GetFetchOption());

  callback(
      GetBlobData(layer_id, kBlobService, data_request, std::move(context)));
  return client::CancellationToken();
}

BlobApi::DataResponse DataRepository::GetVersionedData(
    const std::string& layer_id, const DataRequest& request, int64_t version,
    client::CancellationContext context, const bool fail_on_cache_error) {
  if (request.GetDataHandle() && request.GetPartitionId()) {
    return {{client::ErrorCode::PreconditionFailed,
             "Both data handle and partition id specified"}};
  }

  auto blob_request = request;
  if (!request.GetDataHandle()) {
    // get data handle for a partition to be queried
    PartitionsRepository repository(catalog_, layer_id, settings_,
                                    lookup_client_, storage_);
    auto partitions_response =
        repository.GetPartitionById(request, version, context);

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
  return repository::DataRepository::GetBlobData(
      layer_id, kBlobService, blob_request, std::move(context),
      fail_on_cache_error);
}

///////////////
client::CancellationToken DataRepository::GetVersionedData(
    const std::string& layer_id, const DataRequest& request, int64_t version,
    client::CancellationContext context, bool fail_on_cache_error,
    std::function<void(BlobApi::DataResponse)> callback) {
  //  versioned_data_callback_ = std::move(callback);
  //    partition_repository_.set_layer_id(layer_id);
  if (request.GetDataHandle() && request.GetPartitionId()) {
    callback(client::ApiError{client::ErrorCode::PreconditionFailed,
                              "Both data handle and partition id specified"});
    return client::CancellationToken();
  }

  if (request.GetDataHandle()) {
    auto blob_request = request;
    // finally get the data using a data handle
    return repository::DataRepository::GetBlobData(
        layer_id, kBlobService, blob_request, std::move(context),
        fail_on_cache_error, [=](BlobApi::DataResponse data_response) {
          callback(std::move(data_response));
        });
  }

  // get data handle for a partition to be queried
  //  PartitionsRepository repository(catalog_, layer_id, settings_,
  //  lookup_client_,
  //                                  storage_);
  partition_repository_.set_layer_id(layer_id);
  return partition_repository_.GetPartitionById(
      request, version, [=](read::PartitionsResponse partitions_response) {
        if (!partitions_response.IsSuccessful()) {
          callback(partitions_response.GetError());
          return client::CancellationToken();
        }

        const auto& partitions =
            partitions_response.GetResult().GetPartitions();
        if (partitions.empty()) {
          OLP_SDK_LOG_INFO_F(
              kLogTag,
              "GetVersionedData partition %s not found, hrn='%s', key='%s'",
              request.GetPartitionId() ? request.GetPartitionId().get().c_str()
                                       : "<none>",
              catalog_.ToCatalogHRNString().c_str(),
              request.CreateKey(layer_id, version).c_str());

          callback(client::ApiError{client::ErrorCode::NotFound,
                                    "Partition not found"});
          return client::CancellationToken();
        }

        auto blob_request = request;
        blob_request.WithDataHandle(partitions.front().GetDataHandle());
        // finally get the data using a data handle
        //        callback(repository::DataRepository::GetBlobData(
        //            layer_id, kBlobService, blob_request, std::move(context),
        //            fail_on_cache_error));
        //        return client::CancellationToken();
        //        std::cout << "GetBlob Then 2: " << std::this_thread::get_id()
        //        << std::endl;
        return repository::DataRepository::GetBlobData(
            layer_id, kBlobService, blob_request, std::move(context),
            fail_on_cache_error, [=](BlobApi::DataResponse data_response) {
              //            std::cout << "GetBlob Then 2 finished: " <<
              //            std::this_thread::get_id() << std::endl;
              callback(std::move(data_response));
            });
      });
}

BlobApi::DataResponse DataRepository::GetBlobData(
    const std::string& layer, const std::string& service,
    const DataRequest& request, client::CancellationContext context,
    const bool fail_on_cache_error) {
  auto fetch_option = request.GetFetchOption();
  const auto& data_handle = request.GetDataHandle();

  if (!data_handle) {
    return {{client::ErrorCode::PreconditionFailed, "Data handle is missing"}};
  }

  NamedMutex mutex(storage_, catalog_.ToString() + layer + *data_handle);
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

  // Check if other threads have faced an error.
  const auto optional_error = mutex.GetError();
  if (optional_error) {
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "Found error in NamedMutex, aborting, hrn='%s', "
                        "key='%s', error='%s'",
                        catalog_.ToCatalogHRNString().c_str(),
                        data_handle->c_str(),
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
    storage_response = BlobApi::GetBlob(
        storage_api_lookup.GetResult(), layer, data_handle.value(),
        request.GetBillingTag(), boost::none, context);
  } else {
    auto volatile_blob = VolatileBlobApi::GetVolatileBlob(
        storage_api_lookup.GetResult(), layer, data_handle.value(),
        request.GetBillingTag(), context);
    storage_response = BlobApi::DataResponse(volatile_blob.MoveResult());
  }

  if (storage_response.IsSuccessful() && fetch_option != OnlineOnly) {
    const auto put_result = repository.Put(storage_response.GetResult(), layer,
                                           data_handle.value());
    if (!put_result.IsSuccessful() && fail_on_cache_error) {
      OLP_SDK_LOG_ERROR_F(kLogTag,
                          "Failed to write data to cache, hrn='%s', "
                          "layer='%s', data_handle='%s'",
                          catalog_.ToCatalogHRNString().c_str(), layer.c_str(),
                          data_handle->c_str());
      return put_result.GetError();
    }
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

    // Store an error to share it with other threads.
    mutex.SetError(storage_response.GetError());
  }

  return storage_response;
}

//////////////
client::CancellationToken DataRepository::GetBlobData(
    const std::string& layer, const std::string& service,
    const DataRequest& request, client::CancellationContext context,
    bool fail_on_cache_error,
    std::function<void(BlobApi::DataResponse)> callback) {
  auto fetch_option = request.GetFetchOption();
  auto data_handle = request.GetDataHandle();

  if (!data_handle) {
    callback(client::ApiError{client::ErrorCode::PreconditionFailed,
                              "Data handle is missing"});
    return client::CancellationToken();
  }

  auto llayer = layer;

  NamedMutex mutex(storage_, catalog_.ToString() + layer + *data_handle);
  std::unique_lock<NamedMutex> lock(mutex, std::defer_lock);

  // If we are not planning to go online or access the cache, do not lock.
  if (fetch_option != CacheOnly && fetch_option != OnlineOnly) {
    lock.lock();
  }

  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    auto cached_data = data_repository_.Get(layer, data_handle.value());
    if (cached_data) {
      OLP_SDK_LOG_DEBUG_F(
          kLogTag, "GetBlobData found in cache, hrn='%s', key='%s'",
          catalog_.ToCatalogHRNString().c_str(), data_handle->c_str());
      callback(cached_data.value());
      return client::CancellationToken();
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "GetBlobData not found in cache, hrn='%s', key='%s'",
          catalog_.ToCatalogHRNString().c_str(), data_handle->c_str());
      callback(client::ApiError{client::ErrorCode::NotFound,
                                "CacheOnly: resource not found in cache"});
    }
  }

  // Check if other threads have faced an error.
  const auto optional_error = mutex.GetError();
  if (optional_error) {
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "Found error in NamedMutex, aborting, hrn='%s', "
                        "key='%s', error='%s'",
                        catalog_.ToCatalogHRNString().c_str(),
                        data_handle->c_str(),
                        optional_error->GetMessage().c_str());
    callback(*optional_error);
    return client::CancellationToken();
  }

  return lookup_client_.LookupApi(
      service, "v1", static_cast<client::FetchOptions>(fetch_option),
      [&, callback, llayer, data_handle](
          client::ApiLookupClient::LookupApiResponse storage_api_lookup) {
        if (!storage_api_lookup.IsSuccessful()) {
          // Store an error to share it with other threads.
          mutex.SetError(storage_api_lookup.GetError());

          callback(storage_api_lookup.GetError());
          return client::CancellationToken();
        }

        if (service == kBlobService) {
          return BlobApi::GetBlob(
              storage_api_lookup.GetResult(), layer, data_handle.value(),
              request.GetBillingTag(), boost::none,
              [&, callback, llayer, data_handle,
               fetch_option](BlobApi::DataResponse storage_response) {
                if (storage_response.IsSuccessful() &&
                    fetch_option != OnlineOnly) {
                  auto storage = storage_response.GetResult();
                  callback(storage_response);
                  return client::CancellationToken();
                  //                  callback(client::ApiError{
                  //                      client::ErrorCode::PreconditionFailed,
                  //                      "Both data handle and partition id
                  //                      specified1"});
                  const auto put_result = data_repository_.Put(
                      storage, llayer, data_handle.value());
                  //                  return client::CancellationToken();
                  if (!put_result.IsSuccessful() && fail_on_cache_error) {
                    OLP_SDK_LOG_ERROR_F(
                        kLogTag,
                        "Failed to write data to cache, hrn='%s', "
                        "layer='%s', data_handle='%s'",
                        catalog_.ToCatalogHRNString().c_str(), llayer.c_str(),
                        data_handle->c_str());
                    callback(put_result.GetError());
                    return client::CancellationToken();
                  }
                }

                if (!storage_response.IsSuccessful()) {
                  const auto& error = storage_response.GetError();
                  if (error.GetHttpStatusCode() ==
                      http::HttpStatusCode::FORBIDDEN) {
                    OLP_SDK_LOG_WARNING_F(kLogTag,
                                          "GetBlobData 403 received, remove "
                                          "from cache, hrn='%s', key='%s'",
                                          catalog_.ToCatalogHRNString().c_str(),
                                          data_handle->c_str());
                    data_repository_.Clear(layer, data_handle.value());
                  }

                  // Store an error to share it with other threads.
                  mutex.SetError(storage_response.GetError());
                }

                //                callback(storage_response);
                return client::CancellationToken();
              });
        } else {
          return VolatileBlobApi::GetVolatileBlob(
              storage_api_lookup.GetResult(), layer, data_handle.value(),
              request.GetBillingTag(),
              [&](BlobApi::DataResponse storage_response) {
                if (storage_response.IsSuccessful() &&
                    fetch_option != OnlineOnly) {
                  const auto put_result =
                      data_repository_.Put(storage_response.GetResult(), llayer,
                                           data_handle.value());
                  if (!put_result.IsSuccessful() && fail_on_cache_error) {
                    OLP_SDK_LOG_ERROR_F(
                        kLogTag,
                        "Failed to write data to cache, hrn='%s', "
                        "layer='%s', data_handle='%s'",
                        catalog_.ToCatalogHRNString().c_str(), llayer.c_str(),
                        data_handle->c_str());
                    callback(put_result.GetError());
                    return client::CancellationToken();
                  }
                }

                if (!storage_response.IsSuccessful()) {
                  const auto& error = storage_response.GetError();
                  if (error.GetHttpStatusCode() ==
                      http::HttpStatusCode::FORBIDDEN) {
                    OLP_SDK_LOG_WARNING_F(kLogTag,
                                          "GetBlobData 403 received, remove "
                                          "from cache, hrn='%s', key='%s'",
                                          catalog_.ToCatalogHRNString().c_str(),
                                          data_handle->c_str());
                    data_repository_.Clear(layer, data_handle.value());
                  }

                  // Store an error to share it with other threads.
                  mutex.SetError(storage_response.GetError());
                }

                callback(storage_response);
                return client::CancellationToken();
              });
        }
      });
}

BlobApi::DataResponse DataRepository::GetVolatileData(
    const std::string& layer_id, const DataRequest& request,
    client::CancellationContext context, const bool fail_on_cache_error) {
  if (request.GetDataHandle() && request.GetPartitionId()) {
    return {{client::ErrorCode::PreconditionFailed,
             "Both data handle and partition id specified"}};
  }

  auto blob_request = request;
  if (!request.GetDataHandle()) {
    PartitionsRepository repository(catalog_, layer_id, settings_,
                                    lookup_client_, storage_);
    auto partitions_response =
        repository.GetPartitionById(request, boost::none, context);

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

  return GetBlobData(layer_id, kVolatileBlobService, blob_request,
                     std::move(context), fail_on_cache_error);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
