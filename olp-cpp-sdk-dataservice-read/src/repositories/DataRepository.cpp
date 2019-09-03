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

#include <olp/core/client/Condition.h>
#include "ApiRepository.h"
#include "CatalogRepository.h"
#include "DataCacheRepository.h"
#include "ExecuteOrSchedule.inl"
#include "PartitionsCacheRepository.h"
#include "PartitionsRepository.h"
#include "generated/api/VolatileBlobApi.h"
#include "olp/dataservice/read/CatalogRequest.h"
#include "olp/dataservice/read/DataRequest.h"
#include "olp/dataservice/read/PartitionsRequest.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {
using namespace olp::client;

namespace {
constexpr auto kDataInlinePrefix = "data:";
constexpr auto kLogTag = "DataRepository";

void GetDataInternal(std::shared_ptr<CancellationContext> cancellationContext,
                     std::shared_ptr<ApiRepository> apiRepo,
                     const std::string& layerType,
                     const read::DataRequest& request,
                     const read::DataResponseCallback& callback,
                     DataCacheRepository& cache) {
  std::string service;
  std::function<CancellationToken(const OlpClient&)> dataFunc;
  auto key = request.CreateKey();
  OLP_SDK_LOG_TRACE_F(kLogTag, "GetDataInternal '%s'", key.c_str());
  auto cancel_callback = [callback, key]() {
    OLP_SDK_LOG_INFO_F(kLogTag, "cancelled '%s'", key.c_str());
    callback({{ErrorCode::Cancelled, "Operation cancelled.", true}});
  };

  /* Cache put intercept */
  auto cacheDataResponseCallback = [=, &cache](DataResponse response) {
    if (response.IsSuccessful()) {
      OLP_SDK_LOG_INFO_F(kLogTag, "put '%s' to cache", key.c_str());
      cache.Put(response.GetResult(), request.GetLayerId(),
                request.GetDataHandle().value_or(std::string()));
    } else {
      if (403 == response.GetError().GetHttpStatusCode()) {
        OLP_SDK_LOG_INFO_F(kLogTag, "clear '%s' cache", key.c_str());
        cache.Clear(request.GetLayerId(),
                    request.GetDataHandle().value_or(std::string()));
      }
    }
    callback(response);
  };

  if (layerType == "versioned") {
    service = "blob";
    dataFunc = [=](const OlpClient& client) {
      OLP_SDK_LOG_INFO_F(kLogTag, "getBlob '%s", key.c_str());
      return BlobApi::GetBlob(client, request.GetLayerId(),
                              *request.GetDataHandle(), request.GetBillingTag(),
                              boost::none, cacheDataResponseCallback);
    };
  } else if (layerType == "volatile") {
    service = "volatile-blob";
    dataFunc = [=](const OlpClient& client) {
      OLP_SDK_LOG_INFO_F(kLogTag, "getVolatileBlob '%s", key.c_str());
      return VolatileBlobApi::GetVolatileBlob(
          client, request.GetLayerId(), *request.GetDataHandle(),
          request.GetBillingTag(), cacheDataResponseCallback);
    };
  } else {
    // TODO handle stream api
    OLP_SDK_LOG_INFO_F(kLogTag, "service unavailable '%s'", key.c_str());
    callback(ApiError(client::ErrorCode::ServiceUnavailable,
                      "Stream layers are not supported yet."));
    return;
  }

  cancellationContext->ExecuteOrCancelled(
      [=, &cache]() {
        /* Check the cache */
        if (OnlineOnly != request.GetFetchOption()) {
          auto cachedData =
              cache.Get(request.GetLayerId(),
                        request.GetDataHandle().value_or(std::string()));
          if (cachedData) {
            ExecuteOrSchedule(apiRepo->GetOlpClientSettings(), [=] {
              OLP_SDK_LOG_INFO_F(kLogTag, "cache data '%s' found!",
                                 key.c_str());
              callback(*cachedData);
            });
            return CancellationToken();
          } else if (CacheOnly == request.GetFetchOption()) {
            ExecuteOrSchedule(apiRepo->GetOlpClientSettings(), [=] {
              OLP_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' not found!",
                                 key.c_str());
              callback(
                  ApiError(ErrorCode::NotFound,
                           "Cache only resource not found in cache (data)."));
            });
            return CancellationToken();
          }
        }

        return apiRepo->getApiClient(
            service, "v1", [=](ApiClientResponse response) {
              if (!response.IsSuccessful()) {
                OLP_SDK_LOG_INFO_F(kLogTag, "getApiClient '%s' unsuccessful",
                                   key.c_str());
                callback(response.GetError());
                return;
              }

              cancellationContext->ExecuteOrCancelled(
                  [=]() {
                    OLP_SDK_LOG_INFO_F(kLogTag,
                                       "getApiClient '%s' getting catalog",
                                       key.c_str());
                    return dataFunc(response.GetResult());
                  },
                  cancel_callback);
            });
      },
      cancel_callback);
}

}  // namespace

DataRepository::DataRepository(
    const client::HRN& hrn, std::shared_ptr<ApiRepository> apiRepo,
    std::shared_ptr<CatalogRepository> catalogRepo,
    std::shared_ptr<PartitionsRepository> partitionsRepo,
    std::shared_ptr<cache::KeyValueCache> cache)
    : hrn_(hrn),
      apiRepo_(apiRepo),
      catalogRepo_(catalogRepo),
      partitionsRepo_(partitionsRepo),
      cache_(std::make_shared<DataCacheRepository>(hrn, cache)),
      partitionsCache_(
          std::make_shared<PartitionsCacheRepository>(hrn, cache)) {
  read::DataResponse cancelledResponse{
      {static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
       "Operation cancelled."}};
  multiRequestContext_ =
      std::make_shared<MultiRequestContext<read::DataResponse>>(
          cancelledResponse);
}

bool DataRepository::IsInlineData(const std::string& dataHandle) {
  return (dataHandle.find(kDataInlinePrefix) == 0);
}

CancellationToken DataRepository::GetData(
    const read::DataRequest& request,
    const read::DataResponseCallback& callback) {
  auto key = request.CreateKey();
  OLP_SDK_LOG_TRACE_F(kLogTag, "GetData '%s'", key.c_str());
  if (!request.GetDataHandle() && !request.GetPartitionId()) {
    OLP_SDK_LOG_INFO_F(kLogTag, "getData for '%s' failed", key.c_str());
    callback(ApiError(client::ErrorCode::InvalidArgument,
                      "A data handle or a partition id must be defined."));
    return CancellationToken();
  }

  CatalogRequest catalogRequest;
  catalogRequest.WithBillingTag(request.GetBillingTag())
      .WithFetchOption(request.GetFetchOption());

  // local copy of repos to ensure they live the duration of the request
  auto apiRepo = apiRepo_;
  auto partitionsRepo = partitionsRepo_;
  auto& cache = *cache_;

  auto cancel_context = std::make_shared<CancellationContext>();
  auto cancel_callback = [callback, key]() {
    OLP_SDK_LOG_INFO_F(kLogTag, "cancelled '%s'", key.c_str());
    callback({{ErrorCode::Cancelled, "Operation cancelled.", true}});
  };

  auto executeFn = [=, &cache](read::DataResponseCallback callback) {
    cancel_context->ExecuteOrCancelled(
        [=, &cache]() {
          return catalogRepo_->getCatalog(
              catalogRequest,
              [=, &cache](read::CatalogResponse catalogResponse) {
                if (!catalogResponse.IsSuccessful()) {
                  OLP_SDK_LOG_INFO_F(kLogTag, "getCatalog '%s' unsuccessful",
                                     key.c_str());
                  callback(catalogResponse.GetError());
                  return;
                }

                auto& catalogLayers = catalogResponse.GetResult().GetLayers();
                auto itr =
                    std::find_if(catalogLayers.begin(), catalogLayers.end(),
                                 [&](const model::Layer& layer) {
                                   return layer.GetId() == request.GetLayerId();
                                 });

                if (itr == catalogLayers.end()) {
                  OLP_SDK_LOG_INFO_F(kLogTag, "Layer for '%s' doesn't exiist",
                                     key.c_str());
                  callback(ApiError(client::ErrorCode::InvalidArgument,
                                    "Layer specified doesn't exist."));
                  return;
                }

                auto layerType = itr->GetLayerType();
                if (request.GetDataHandle()) {
                  GetDataInternal(cancel_context, apiRepo, layerType, request,
                                  callback, cache);
                } else {
                  auto getPartitionsCallback = [=, &cache](
                                                   PartitionsResponse
                                                       partitionsResponse) {
                    if (!partitionsResponse.IsSuccessful()) {
                      callback(partitionsResponse.GetError());
                      return;
                    }

                    if (partitionsResponse.GetResult().GetPartitions().size() ==
                        1) {
                      auto& dataHandle = partitionsResponse.GetResult()
                                             .GetPartitions()
                                             .at(0)
                                             .GetDataHandle();
                      if (IsInlineData(dataHandle)) {
                        callback(std::make_shared<std::vector<unsigned char>>(
                            dataHandle.begin(), dataHandle.end()));
                      } else {
                        DataRequest appendedRequest(request);
                        appendedRequest.WithDataHandle(
                            partitionsResponse.GetResult()
                                .GetPartitions()
                                .at(0)
                                .GetDataHandle());
                        auto verifyResponseCallback =
                            [=](DataResponse response) {
                              if (!response.IsSuccessful()) {
                                if (403 ==
                                    response.GetError().GetHttpStatusCode()) {
                                  PartitionsRequest partitionsRequest;
                                  partitionsRequest
                                      .WithBillingTag(
                                          appendedRequest.GetBillingTag())
                                      .WithLayerId(appendedRequest.GetLayerId())
                                      .WithVersion(
                                          appendedRequest.GetVersion());
                                  std::vector<std::string> partitions;
                                  partitions.push_back(
                                      *appendedRequest.GetPartitionId());
                                  partitionsCache_->ClearPartitions(
                                      partitionsRequest, partitions);
                                }
                              }
                              callback(response);
                            };
                        GetDataInternal(cancel_context, apiRepo, layerType,
                                        appendedRequest, verifyResponseCallback,
                                        cache);
                      }
                    } else {
                      // Backend returns an empty partition list if the
                      // partition doesn't exist in the layer. So return no
                      // data.
                      OLP_SDK_LOG_INFO_F(kLogTag, "Empty partition for '%s'!",
                                         key.c_str());
                      callback(model::Data());
                    }
                  };

                  cancel_context->ExecuteOrCancelled(
                      [=]() {
                        PartitionsRequest partitionsRequest;
                        partitionsRequest
                            .WithBillingTag(request.GetBillingTag())
                            .WithFetchOption(request.GetFetchOption())
                            .WithLayerId(request.GetLayerId())
                            .WithVersion(request.GetVersion());
                        std::vector<std::string> partitions;
                        partitions.push_back(*request.GetPartitionId());
                        return partitionsRepo->GetPartitionsById(
                            partitionsRequest, partitions,
                            getPartitionsCallback);
                      },

                      cancel_callback);
                }
              });
        },

        cancel_callback);

    return CancellationToken(
        [cancel_context]() { cancel_context->CancelOperation(); });
  };
  return multiRequestContext_->ExecuteOrAssociate(key, executeFn, callback);
}

void DataRepository::GetData(
    std::shared_ptr<client::CancellationContext> cancellationContext,
    const std::string& layerType, const read::DataRequest& request,
    const read::DataResponseCallback& callback) {
  GetDataInternal(cancellationContext, apiRepo_, layerType, request, callback,
                  *cache_);
}

DataRepository::DataResponse DataRepository::GetVersionedData(
    HRN catalog, std::string layer_id, OlpClientSettings client_settings,
    DataRequest data_request, CancellationContext context) {
  if (!data_request.GetDataHandle()) {
    if (!data_request.GetVersion()) {
      // get latest version of the layer if it wasn't set by the user
      auto latest_version_response =
          repository::CatalogRepository::GetLatestVersion(
              catalog, context, data_request, client_settings);
      if (!latest_version_response.IsSuccessful()) {
        return latest_version_response.GetError();
      }
      data_request.WithVersion(
          latest_version_response.GetResult().GetVersion());
    }

    // get data handle for a partition to be queried
    auto partitions_response =
        repository::PartitionsRepository::GetPartitionById(
            catalog, layer_id, context, data_request, client_settings);
    if (!partitions_response.IsSuccessful()) {
      return partitions_response.GetError();
    }
    auto partitions = partitions_response.GetResult().GetPartitions();
    if (partitions.empty()) {
      return client::ApiError(client::ErrorCode::NotFound,
                              "Partition not found");
    }
    data_request.WithDataHandle(partitions.front().GetDataHandle());
  }

  // finally get the data using a data handle
  auto data_response = repository::DataRepository::GetBlobData(
      catalog, layer_id, "blob", data_request, context, client_settings);
  return data_response;
}

DataRepository::DataResponse DataRepository::GetBlobData(
    const client::HRN& catalog, const std::string& layer,
    const std::string& service, const DataRequest& data_request,
    client::CancellationContext cancellation_context,
    client::OlpClientSettings settings) {
  using namespace client;

  auto fetch_option = data_request.GetFetchOption();
  const auto& data_handle = data_request.GetDataHandle();

  if (!data_handle) {
    return ApiError(ErrorCode::PreconditionFailed, "Data handle is missing");
  }

  repository::DataCacheRepository repository(catalog, settings.cache);

  if (fetch_option != OnlineOnly) {
    auto cached_data = repository.Get(layer, data_handle.value());
    if (cached_data) {
      OLP_SDK_LOG_INFO_F(kLogTag, "cache data '%s' found!",
                         data_request.CreateKey().c_str());
      return cached_data.value();
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(kLogTag, "cache data '%s' not found!",
                         data_request.CreateKey().c_str());
      return ApiError(ErrorCode::NotFound,
                      "Cache only resource not found in cache (data).");
    }
  }

  auto blob_api = ApiClientLookup::LookupApi(
      catalog, cancellation_context, service, "v1", fetch_option, settings);

  if (!blob_api.IsSuccessful()) {
    return blob_api.GetError();
  }

  const client::OlpClient& client = blob_api.GetResult();

  Condition condition;

  // when the network operation took too much time we cancel it and exit
  // execution, to make sure that network callback will not access dangling
  // references we protect them with atomic bool flag.
  auto flag = std::make_shared<std::atomic_bool>(true);

  BlobApi::DataResponse blob_response;
  auto blob_callback = [&, flag](BlobApi::DataResponse response) {
    if (flag->exchange(false)) {
      blob_response = std::move(response);
      condition.Notify();
    }
  };

  cancellation_context.ExecuteOrCancelled(
      [&]() {
        client::CancellationToken token;
        if (service == "blob") {
          token = BlobApi::GetBlob(client, layer, data_handle.value(),
                                   data_request.GetBillingTag(), boost::none,
                                   std::move(blob_callback));
        } else {
          token = VolatileBlobApi::GetVolatileBlob(
              client, layer, data_handle.value(), data_request.GetBillingTag(),
              std::move(blob_callback));
        }

        return client::CancellationToken([&, token, flag]() {
          if (flag->exchange(false)) {
            token.cancel();
            condition.Notify();
          }
        });
      },
      [&]() {
        // if context was cancelled before the execution setup, unblock the
        // upcoming wait routine.
        condition.Notify();
      });

  if (!condition.Wait()) {
    // We are just about exit the execution.

    cancellation_context.CancelOperation();
    return ApiError(ErrorCode::RequestTimeout, "Network request timed out.");
  }

  flag->store(false);

  if (cancellation_context.IsCancelled()) {
    // We can't use blob response here because it could potentially be
    // uninitialized.
    return ApiError(ErrorCode::Cancelled, "Operation cancelled.");
  }

  if (blob_response.IsSuccessful()) {
    repository.Put(blob_response.GetResult(), layer, data_handle.value());
  } else {
    const auto& error = blob_response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_INFO_F(kLogTag, "clear '%s' cache",
                         data_request.CreateKey().c_str());
      repository.Clear(layer, data_handle.value());
    }
  }

  return blob_response;
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
